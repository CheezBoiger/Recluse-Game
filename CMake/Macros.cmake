# Copyright (c) 2017 Recluse Project. All rights reserved.

# From https://stackoverflow.com/questions/697560/how-to-copy-directory-from-source-tree-to-binary-tree
macro(configure_files src dst)
  message(STATUS "Configuring directory ${dst}")
  make_directory(${dst})

  file(GLOB templateFiles RELATIVE ${src} ${src}/*)
  foreach(templateFile ${templateFiles})
    set(srcTemplatePath ${src}/${templateFile})
    if (NOT IS_DIRECTORY ${srcTemplateFile})
      message(STATUS "Configuring file ${templateFile}")
      configure_file(${srcTemplatePath} ${dst}/${templateFile} @ONLY)
    endif()
  endforeach()
endmacro()