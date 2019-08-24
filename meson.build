project('tx', 'c')

sdl2image = meson.get_compiler('c').find_library('SDL2_image')
sdl2image = declare_dependency(dependencies: [sdl2image])

executable('tx', 'main.c', dependencies: [
    dependency('epoxy'),
    dependency('sdl2'),
    sdl2image
])
