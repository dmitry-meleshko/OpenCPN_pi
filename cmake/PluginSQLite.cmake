##---------------------------------------------------------------------------
## Author:      based on work of Sean D'Epagnier
## Copyright:   
## License:     GPLv3+
##---------------------------------------------------------------------------

# configure SQLite library

IF(WIN32)
  IF(MSVC)
    SET(SQLITE_LIBRARIES "../buildwin/sqlite3")
    INSTALL(FILES "buildwin/sqlite3.dll" DESTINATION ".")
  ELSE(MSVC) ## mingw
    SET(SQLITE_LIBRARIES "sqlite3.dll")
    INSTALL(FILES "buildwin/sqlite3.dll" DESTINATION ".")
  ENDIF(MSVC)
    
  TARGET_LINK_LIBRARIES(${PACKAGE_NAME} ${SQLITE_LIBRARIES})
  INCLUDE_DIRECTORIES(src/sqlite)
ENDIF(WIN32)
