/*! @file ExecutableInfo.h
 *  @brief ExecutableInfo Specification
 *
 *  This file contains the specification of the ExecutableInfo class.
 */

#ifndef __EXECUTABLEINFO_H__
#define __EXECUTABLEINFO_H__

#include <map>
#include <stdint.h>
#include <string>

#include "CoverageMapBase.h"
#include "SymbolTable.h"

namespace Coverage {

  /*! @class ExecutableInfo
   *
   *  This class holds a collection of information for an executable
   *  that is to be analyzed.
   */
  class ExecutableInfo {

  public:

    /*!
     *  This method constructs an ExecutableInfo instance.
     *
     *  @param[in] theExecutableName specifies the name of the executable
     *  @param[in] theLibraryName specifies the name of the executable
     */
    ExecutableInfo(
      const char* const theExecutableName,
      const char* const theLibraryName = NULL
    );

    /*!
     *  This method destructs an ExecutableInfo instance.
     */
    virtual ~ExecutableInfo();

    /*!
     *  This method prints the contents of all coverage maps for
     *  this executable.
     */
    void dumpCoverageMaps( void );

    /*!
     *  This method prints the contents of Executable info containers
     */
    void dumpExecutableInfo( void );

    /*!
     *  This method returns a pointer to the executable's coverage map
     *  that contains the specified address.
     *
     *  @param[in] address specifies the desired address
     *
     *  @return Returns a pointer to the coverage map
     */
    CoverageMapBase* getCoverageMap( uint32_t address );

    /*!
     *  This method returns the file name of the executable.
     *
     *  @return Returns the executable's file name
     */
    std::string getFileName( void ) const;

    /*!
     *  This method returns the library name associated with the executable.
     *
     *  @return Returns the executable's library name
     */
    std::string getLibraryName( void ) const;

    /*!
     *  This method returns the load address of the dynamic library
     *
     *  @return Returns the load address of the dynamic library
     */
    uint32_t getLoadAddress( void ) const;

    /*!
     *  This method returns a pointer to the executable's symbol table.
     *
     *  @return Returns a pointer to the symbol table.
     */
    SymbolTable* getSymbolTable( void ) const;

    /*!
     *  This method creates a coverage map for the specified symbol.
     *
     *  @param[in] exefileName specifies the source of the information
     *  @param[in] symbolName specifies the name of the symbol
     *  @param[in] lowAddress specifies the low address of the coverage map
     *  @param[in] highAddress specifies the high address of the coverage map
     *
     *  @return Returns a pointer to the coverage map
     */
    CoverageMapBase* createCoverageMap (
      const std::string& exefileName,
      const std::string& symbolName,
      uint32_t           lowAddress,
      uint32_t           highAddress
    );

    /*!
     *  This method indicates whether a dynamic library has been
     *  associated with the executable.
     *
     *  @return Returns TRUE if 
     */
    bool hasDynamicLibrary( void );

    /*!
     *  This method merges the coverage maps for this executable into
     *  the unified coverage map.
     */
    void mergeCoverage( void );

    /*!
     *  This method sets the load address of the dynamic library
     *
     *  @param[in] address specifies the load address of the dynamic
     *             library
     */
    void setLoadAddress( uint32_t address );

  private:

    /*!
     *  This map associates a symbol with its coverage map.
     */
    typedef std::map<std::string, CoverageMapBase *> coverageMaps_t;
    coverageMaps_t coverageMaps;

    /*!
     *  This member variable contains the name of the executable.
     */
    std::string executableName;

    /*!
     *  This member variable contains the name of a dynamic library
     *  associated with the executable.
     */
    std::string libraryName;

    /*!
     *  This member variable contains the load address of a dynamic library
     *  if one has been specified for the executable.
     */
    uint32_t loadAddress;

    /*!
     *  This member variable contains a pointer to the symbol table
     *  of the executable or library.
     */
    SymbolTable* theSymbolTable;

  };
}
#endif
