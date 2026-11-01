# Uses its own FFmpeg (9.x) copy, separate from the one in napvideo (distinct variable names avoid clashing cache entries)

set(AACDEC_FFMPEG_DIR ${NAP_ROOT}/modules/napaacdecode/thirdparty/ffmpeg/linux/${ARCH})
set(AACDEC_FFMPEG_INCLUDE_DIR ${AACDEC_FFMPEG_DIR}/include)
set(AACDEC_FFMPEG_LIBRARIES "")
foreach(ffmpeg_lib avformat avcodec swresample avutil)
    find_library(AACDEC_FFMPEG_${ffmpeg_lib} NAMES ${ffmpeg_lib} PATHS ${AACDEC_FFMPEG_DIR}/lib NO_DEFAULT_PATH)
    if(NOT AACDEC_FFMPEG_${ffmpeg_lib})
        message(FATAL_ERROR "napaacdecode: could not find lib${ffmpeg_lib} in ${AACDEC_FFMPEG_DIR}/lib")
    endif()
    list(APPEND AACDEC_FFMPEG_LIBRARIES ${AACDEC_FFMPEG_${ffmpeg_lib}})
endforeach()

set(ffmpeg_dest_dir modules/napaacdecode/thirdparty/ffmpeg)

if(NAP_BUILD_CONTEXT MATCHES "source")
    # Our FFmpeg headers must come before any other FFmpeg headers on the include path
    target_include_directories(${PROJECT_NAME} BEFORE PUBLIC ${AACDEC_FFMPEG_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${AACDEC_FFMPEG_LIBRARIES})

    # Package FFmpeg into platform release
    install(DIRECTORY ${AACDEC_FFMPEG_DIR}/ DESTINATION ${ffmpeg_dest_dir}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})
else()
    # Our FFmpeg headers must come before any other FFmpeg headers on the include path
    target_include_directories(${PROJECT_NAME} BEFORE PUBLIC ${AACDEC_FFMPEG_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${AACDEC_FFMPEG_LIBRARIES})

    set(MODULE_EXTRA_LIBS ${AACDEC_FFMPEG_LIBRARIES})

    # Install FFmpeg into packaged app
    install(DIRECTORY ${NAP_ROOT}/${ffmpeg_dest_dir}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/lib/ DESTINATION lib)
endif()
