project('virtual-led-ctrl', 'cpp',
    version: '1.0',
    default_options: [
        'warning_level=3',
        'cpp_std=c++23'
    ]
)

sdbusplus_dep = dependency('sdbusplus')
boost_dep = dependency('boost')
libgpiod_cxx_dep = dependency('libgpiodcxx')

executable(
    'virtual-led-ctrl',
    'src/main.cpp',
    dependencies: [
        sdbusplus_dep,
        boost_dep,
        libgpiod_cxx_dep
    ],
    install: true,
    install_dir: get_option('bindir')
)
