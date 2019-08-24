#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define HANDMADE_IMPLEMENTATION
#include "HandmadeMath.h"

#include "source_code_pro.h"

#define nil NULL

typedef const char* str;
typedef char* string;

typedef struct {
    GLuint id, m4, tx, vao, vbo, pos;
} Shader;

void sdl_die_if(int test, str msg) {
    if (test) {
        printf("sdl %s error: %s\n", msg, SDL_GetError());
        exit(1);
    }
}

GLuint gl_load_texture(str fpath) {
    SDL_Surface *img = IMG_Load(fpath);
    sdl_die_if(!img, "img load");

    GLuint id;
    glGenTextures(1, &id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

    SDL_FreeSurface(img);

    return id;
}

GLuint gl_check_status(GLuint id, GLenum type) {
    GLint status;
    type == GL_COMPILE_STATUS ? glGetShaderiv(id, type, &status) : glGetProgramiv(id, type, &status);
    if (status)
        return id;

    string err, info;
    GLint len;
    if (type == GL_COMPILE_STATUS) { 
        err = "compile shader";
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        info = (string)malloc(len);
        glGetShaderInfoLog(id, len, nil, info);
    } else {
        err = "link program";
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
        info = (string)malloc(len);
        glGetProgramInfoLog(id, len, nil, info);
    }
    printf("gl %s error:\n%s", err, info);
    free(info);

    exit(1);
}

GLuint vao_() {   
    GLuint id;
    glGenVertexArrays(1, &id);
    return id;
}

GLuint vbo_() {   
    GLuint id;
    glGenBuffers(1, &id);
    return id;
}

string read_file(str path) {
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");
    sdl_die_if(!rw, "open file");

    Sint64 size = SDL_RWsize(rw);
    string buf = (string)calloc(size + 1, sizeof(char));
    SDL_RWread(rw, buf, sizeof(char), size);
    SDL_RWclose(rw);
    return buf;
}

GLuint gl_load_shader(str name, GLenum type) {
    string path;
    asprintf(&path, "../%s_%c.glsl", name, type == GL_VERTEX_SHADER ? 'v' : 'f');

    str src = read_file(path);
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nil); 
    glCompileShader(id);
    free((void *)src);

    return gl_check_status(id, GL_COMPILE_STATUS);
}

Shader shader_(str name) {
    GLuint
        id = glCreateProgram(),
        vs = gl_load_shader(name, GL_VERTEX_SHADER),
        fs = gl_load_shader(name, GL_FRAGMENT_SHADER);
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);
    gl_check_status(id, GL_LINK_STATUS);
    glDetachShader(id, vs);
    glDetachShader(id, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return (Shader){
        .id = id,
        .vao = vao_(),
        .vbo = vbo_()
    };
}

void del_shader(Shader *s) {
    glDeleteBuffers(1, &s->vbo);
    glDeleteVertexArrays(1, &s->vao);
    glDeleteProgram(s->id);
}

void render_font(Shader *s, str text, float x, float y) {
    const size_t len = strlen(text);
    GLfloat xys[len * 24];
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n') {
            x = 0.0;
            y += 38.0; // line height, base, padding ?
            continue;
        }

        const int *c = source_code_pro[(int)text[i]];
        const float
            cx = c[0],
            cy = c[1],
            w = c[2],
            h = c[3],
            xoffset = c[4],
            yoffset = c[5],
            xadvance = c[6];

        // screen coords
        const float
            rat = 0.5, // 16px == 12pt
            x1 = rat * (x + xoffset),
            y1 = rat * (y + yoffset + h),
            x2 = rat * (x + w),
            y2 = rat * (y + yoffset);
        x += xadvance;

        // texture coords
        const float
            x3 = cx / 512.0,
            y3 = (cy + h) / 512.0,
            x4 = (cx + w) / 512.0,
            y4 = cy / 512.0;

        const float xys1[24] = {
            x1, y1, x3, y3,
            x2, y1, x4, y3,
            x2, y2, x4, y4,
            x1, y1, x3, y3,
            x2, y2, x4, y4,
            x1, y2, x3, y4
        };
        for (size_t j = 0; j < 24; j++) {
            xys[i * 24 + j] = xys1[j];
        }
    } 

    glUseProgram(s->id);
    glUniformMatrix4fv(s->m4, 1, GL_FALSE, &HMM_Orthographic(0.0, 640.0, 480.0, 0.0, 0.0, 1.0).Elements[0][0]);
    glUniform1i(s->tx, 0);

    glBindVertexArray(s->vao); 

    glBindBuffer(GL_ARRAY_BUFFER, s->vbo); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * len * 24, xys, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(s->pos); 
    glVertexAttribPointer(s->pos, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLES, 0, 6 * len); 

    glDisable(GL_BLEND);
    glDisableVertexAttribArray(s->pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

int main() {
    sdl_die_if(SDL_Init(SDL_INIT_VIDEO), "init");

    SDL_Window *win = SDL_CreateWindow(
        "tx", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        640, 
        480, 
        SDL_WINDOW_OPENGL
    );
    sdl_die_if(!win, "create window");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GLContext *ctx = SDL_GL_CreateContext(win);
    sdl_die_if(!ctx, "gl create context");

    sdl_die_if(SDL_GL_SetSwapInterval(1) < 0, "gl set swap interval");

    int flags = IMG_INIT_PNG;
    sdl_die_if((IMG_Init(flags) & flags) != flags, "img init");

    glClearColor(0.0, 0.0, 0.0, 1.0);

    Shader font_gl = shader_("font");
    font_gl.tx = 1;
    gl_load_texture("source_code_pro.png");

    string src = read_file("../main.c");

    SDL_Event evt;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) {
                running = 0;
                break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        render_font(&font_gl, src, 0, 0);

        SDL_GL_SwapWindow(win);
    }

    free(src);
    del_shader(&font_gl);
    IMG_Quit();
    SDL_DestroyWindow(win);
    SDL_Quit();
}
