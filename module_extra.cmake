set(FFMPEG_FIND_QUIETLY TRUE)
find_package(FFmpeg REQUIRED)

set(ffmpeg_dest_dir modules/napaacdecode/thirdparty/ffmpeg)
if(NAP_BUILD_CONTEXT MATCHES "source")
    target_include_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_INCLUDE_DIR})

    target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})

    ## Package into platform release

    # Package dependencies into release --
    install(DIRECTORY ${FFMPEG_LICENSE_DIR} DESTINATION ${ffmpeg_dest_dir})
    install(FILES ${FFMPEG_SOURCE_DIST} DESTINATION ${ffmpeg_dest_dir})
    set(FFMPEG_DIR ${FFMPEG_INCLUDE_DIR}/..)
    install(DIRECTORY ${FFMPEG_DIR}/ DESTINATION ${ffmpeg_dest_dir}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})

    if(UNIX AND NOT UNIX)
        # Let our installed FFmpeg libs find each other on Linux
        install(CODE "file(GLOB FFMPEG_DYLIBS ${CMAKE_INSTALL_PREFIX}/${ffmpeg_dest_dir}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/lib/lib*${CMAKE_SHARED_LIBRARY_SUFFIX}*)
                      foreach(FFMPEG_DYLIB \${FFMPEG_DYLIBS})
                          execute_process(COMMAND patchelf --set-rpath \$ORIGIN/. \${FFMPEG_DYLIB}
                                          RESULT_VARIABLE EXIT_CODE
                                          )
                          if(NOT \${EXIT_CODE} EQUAL 0)
                              message(FATAL_ERROR \"Failed to add inter-FFmpeg RPATHs on ${FFMPEG_DYLIB}\")
                          endif()
                      endforeach()
                      ")
    endif()
else()
    set(MODULE_EXTRA_LIBS ${FFMPEG_LIBRARIES})

    # Install FFmpeg into packaged app
    install(DIRECTORY ${NAP_ROOT}/${ffmpeg_dest_dir}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/lib/ DESTINATION lib)

    # Install FFmpeg license into packaged app
    install(FILES ${FFMPEG_LICENSE_DIR}/COPYING.LGPLv2.1
            ${FFMPEG_LICENSE_DIR}/COPYING.LGPLv3
            ${FFMPEG_LICENSE_DIR}/LICENSE.md
            DESTINATION licenses/FFmpeg
    )
    # Install FFmpeg source into packaged app to comply with license
    install(FILES ${FFMPEG_SOURCE_DIST} DESTINATION licenses/FFmpeg)
endif()
