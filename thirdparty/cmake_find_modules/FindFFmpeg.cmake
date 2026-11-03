
if(FFMPEG_LIBRARIES_AAC AND FFMPEG_INCLUDE_DIR_AAC)
    # in cache already
    set(FFMPEG_FOUND_AAC TRUE)

else(FFMPEG_LIBRARIES_AAC AND FFMPEG_INCLUDE_DIR_AAC)
    set(FFMPEG_ROOT_DIR_AAC ${NAP_ROOT}/modules/napaacdecode/thirdparty/ffmpeg)

    set(FFMPEG_DIR_AAC ${FFMPEG_ROOT_DIR_AAC}/linux/${ARCH})
    find_path(FFMPEG_AVCODEC_INCLUDE_DIR_AAC
              NAMES libavcodec/avcodec.h
              HINTS ${FFMPEG_DIR_AAC}/include
              )
    # Licenses
    find_path(FFMPEG_LICENSE_DIR_AAC
        NAMES LICENSE.md
        PATHS ${FFMPEG_ROOT_DIR_AAC}/licenses
        )

    find_library(FFMPEG_LIBAVCODEC_AAC
                 NAMES avcodec-aac
                 PATHS ${FFMPEG_ROOT_DIR_AAC}/linux/${ARCH}/lib
                 NO_DEFAULT_PATH
                 )

    find_library(FFMPEG_LIBAVFORMAT_AAC
                 NAMES avformat-aac
                 PATHS ${FFMPEG_ROOT_DIR_AAC}/linux/${ARCH}/lib
                 NO_DEFAULT_PATH
                 )

    find_library(FFMPEG_LIBAVUTIL_AAC
                 NAMES avutil-aac
                 PATHS ${FFMPEG_ROOT_DIR_AAC}/linux/${ARCH}/lib
                 NO_DEFAULT_PATH
                 )

    find_library(FFMPEG_SWRESAMPLE_AAC
                 NAMES swresample-aac
                 PATHS ${FFMPEG_ROOT_DIR_AAC}/linux/${ARCH}/lib
                 NO_DEFAULT_PATH
                 )

    if(FFMPEG_LIBAVCODEC_AAC AND FFMPEG_LIBAVFORMAT_AAC)
        set(FFMPEG_FOUND_AAC TRUE)
    endif()

    if(FFMPEG_FOUND_AAC)
        set(FFMPEG_INCLUDE_DIR_AAC ${FFMPEG_AVCODEC_INCLUDE_DIR_AAC})

        set(FFMPEG_LIBRARIES_AAC
            ${FFMPEG_LIBAVCODEC_AAC}
            ${FFMPEG_LIBAVFORMAT_AAC}
            ${FFMPEG_LIBAVUTIL_AAC}
            ${FFMPEG_SWRESAMPLE_AAC}
            )
    endif(FFMPEG_FOUND_AAC)

    if(FFMPEG_FOUND_AAC)
        if(NOT FFMPEG_FIND_QUIETLY)
            message(STATUS "Found FFmpeg or Libav: ${FFMPEG_LIBRARIES_AAC}, ${FFMPEG_INCLUDE_DIR_AAC}")
        endif(NOT FFMPEG_FIND_QUIETLY)
    else(FFMPEG_FOUND_AAC)
        if(FFMPEG_FIND_REQUIRED)
            message(FATAL_ERROR "Could not find libavcodec or libavformat or libavutil")
        endif(FFMPEG_FIND_REQUIRED)
    endif(FFMPEG_FOUND_AAC)

endif(FFMPEG_LIBRARIES_AAC AND FFMPEG_INCLUDE_DIR_AAC)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg_AAC REQUIRED_VARS FFMPEG_LIBRARIES_AAC FFMPEG_SOURCE_DIST FFMPEG_LICENSE_DIR_AAC)
