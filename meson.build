project(
    'MlActions',
    'c', 'cpp',
    default_options : ['cpp_std=c++17', 'c_std=c11', 'buildtype=debugoptimized'],
    license : 'MIT',
    version : '0.0.5'
)

add_global_arguments('-DDEBUG', language : 'c')
add_global_arguments('-DDEBUG', language : 'cpp')

if meson.get_compiler('cpp').get_id() == 'clang'
    message('Using Clang compiler for C++, enabling all warnings')
    add_global_arguments('-Weverything', '-Wno-c++98-compat', '-Wno-c++98-compat-pedantic', '-Wno-padded', '-Wno-exit-time-destructors', '-Wno-global-constructors', '-Wno-newline-eof', '-Wstrict-overflow=3', language : 'cpp')
endif

if meson.get_compiler('c').get_id() == 'clang'
    message('Using Clang compiler for C, enabling all warnings')
    add_global_arguments('-Weverything', language : 'c')
endif

srcs = [
    'src/MlActions.cpp'
]

test_executable = executable('tests', sources : [
    'src/MlActions.cpp',
    'src/test.cpp'
])
test('tests', test_executable)
