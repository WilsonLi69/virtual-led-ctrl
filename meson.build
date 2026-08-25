project('virtual-led-ctrl', 'cpp',
    version: '1.0',
    default_options: [
        'warning_level=3',
        'cpp_std=c++20'
    ]
)

sdbusplus_dep = dependency('sdbusplus')

executable(
    'virtual-led-ctrl',
    'src/main.cpp',
    dependencies: [
        sdbusplus_dep,
    ],
    install: true,
    install_dir: get_option('bindir')
)
