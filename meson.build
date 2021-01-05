project('ImageTest', 'cpp',
  version : '0.1',
  default_options : ['warning_level=1',
                     'cpp_std=c++14'])

add_project_arguments('-DLOTTIE_RESOURCE_DIR="@0@/resource/lottie/"'.format(meson.current_source_dir()), language : 'cpp')
add_project_arguments('-DLOTTIE_BASELINE_DIR="@0@/baseline/lottie/"'.format(meson.current_source_dir()), language : 'cpp')

add_project_arguments('-DSVG_RESOURCE_DIR="@0@/resource/svg/"'.format(meson.current_source_dir()), language : 'cpp')
add_project_arguments('-DSVG_BASELINE_DIR="@0@/baseline/svg/"'.format(meson.current_source_dir()), language : 'cpp')

lib_dep = [dependency('rlottie', required : true), dependency('thorvg', required : true)]

exe = executable('imagetest', 'imagetest.cpp',
				 dependencies : lib_dep,
  				 install : true)

test('basic', exe)
