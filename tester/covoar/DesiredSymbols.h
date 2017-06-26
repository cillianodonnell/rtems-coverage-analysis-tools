/*! @file DesiredSymbols.h
 *  @brief DesiredSymbols Specification
 *
 *  This file contains the specification of the DesiredSymbols class.
 */

#ifndef __DESIRED_SYMBOLS_H__
#define __DESIRED_SYMBOLS_H__

#include <list>
#include <map>
#include <stdint.h>
#include <string>

#include "CoverageMapBase.h"
#include "CoverageRanges.h"
#include "ExecutableInfo.h"
#include "ObjdumpProcessor.h"

namespace Coverage {


  /*! 
   * 
   *  This class defines the statistics that are tracked.
   */
  class Statistics {
    public:

    /*!
     *  This member variable contains the total number of branches always
     *  taken.
     */
    int branchesAlwaysTaken;

    /*!
     *  This member variable contains the total number of branches where 
     *  one or more paths were executed.
     */
    int branchesExecuted;

    /*!
     *  This member variable contains the total number of branches never
     *  taken.
     */
    int branchesNeverTaken;

    /*!
     *  This member variable contains the total number of branches not
     *  executed AT ALL.
     */
    int branchesNotExecuted;

    /*!
     *  This member contains the size in Bytes.
     */
    uint32_t sizeInBytes;
    
    /*!
     *  This member contains the size in Bytes.
     */
    uint32_t sizeInInstructions;

    /*!
     *  This member variable contains the total number of uncovered bytes.
     */
    int uncoveredBytes;

    /*!
     *  This member variable contains the total number of uncovered assembly
     *  instructions.
     */
    int uncoveredInstructions;

    /*!
     *  This member variable contains the total number of uncovered ranges.
     */
    int uncoveredRanges;

    /*!
     *  This method returns the percentage of uncovered instructions.
     *
     *  @return Returns the percent uncovered instructions
     */
    uint32_t getPercentUncoveredInstructions( void ) const;

    /*!
     *  This method returns the percentage of uncovered bytes.
     *
     *  @return Returns the percent uncovered bytes
     */
     uint32_t getPercentUncoveredBytes( void ) const;

    /*!
     *  This method constructs a Statistics instance.
     */   
     Statistics():
       branchesAlwaysTaken(0),
       branchesExecuted(0),
       branchesNeverTaken(0),
       branchesNotExecuted(0),
       sizeInBytes(0),
       sizeInInstructions(0),
       uncoveredBytes(0),
       uncoveredInstructions(0),
       uncoveredRanges(0)
     {
     }

  };

  /*! @class SymbolInformation
   *
   *  This class defines the information kept for each symbol that is
   *  to be analyzed.
   */
  class SymbolInformation {

  public:

    /*!
     *  This member contains the base address of the symbol.
     */
    uint32_t baseAddress;


    /*!
     *  This member contains the disassembly associated with a symbol.
     */
    std::list<ObjdumpProcessor::objdumpLine_t> instructions;

    /*!
     *  This member contains the executable that was used to
     *  generate the disassembled instructions.
     */
    ExecutableInfo* sourceFile;

    /*!
     *  This member contains the statistics kept on each symbol.
     */    
    Statistics stats;

    /*!
     *  This member contains information about the branch instructions of
     *  a symbol that were not fully covered (i.e. taken/not taken).
     */
    CoverageRanges* uncoveredBranches;

    /*!
     *  This member contains information about the instructions of a
     *  symbol that were not executed.
     */
    CoverageRanges* uncoveredRanges;

    /*!
     *  This member contains the unified or merged coverage map
     *  for the symbol.
     */
    CoverageMapBase* unifiedCoverageMap;

    /*!
     *  This method constructs a SymbolInformation instance.
     */
    SymbolInformation() :
      baseAddress( 0 ),
      uncoveredBranches( NULL ),
      uncoveredRanges( NULL ),
      unifiedCoverageMap( NULL )
    {
    }

    ~SymbolInformation() {}
  };

  /*! @class DesiredSymbols
   *
   *  This class defines the set of desired symbols to analyze.
   */
  class DesiredSymbols {

  public:

    /*!
     *  This map associates each symbol with its symbol information.
     */
    typedef std::map<std::string, SymbolInformation> symbolSet_t;

    /*!
     *  This variable contains a map of symbol sets for each 
     *  symbol in the system keyed on the symbol name.
     */
    symbolSet_t set;

    /*! 
     *  This method constructs a DesiredSymbols instance.
     */
    DesiredSymbols();

    /*! 
     *  This method destructs a DesiredSymbols instance.
     */
    ~DesiredSymbols();

    /*!
     *  This method loops through the coverage map and
     *  calculates the statistics that have not already 
     *  been filled in.
     */
    void calculateStatistics( void );

    /*!
     *  This method analyzes each symbols coverage map to determine any
     *  uncovered ranges or branches.
     */
    void computeUncovered( void );

    /*!
     *  This method creates a coverage map for the specified symbol
     *  using the specified size.
     *
     *  @param[in] exefileName specifies the executable from which the
     *             coverage map is being created
     *  @param[in] symbolName specifies the symbol for which to create
     *             a coverage map
     *  @param[in] size specifies the size of the coverage map to create
     */
    void createCoverageMap(
      const std::string& exefileName,
      const std::string& symbolName,
      uint32_t           size
    );

    /*!
     *  This method looks up the symbol information for the specified symbol.
     *
     *  @param[in] symbolName specifies the symbol for which to search
     *
     *  @return Returns a pointer to the symbol's information
     */
    SymbolInformation* find(
      const std::string& symbolName
    );

    /*!
     *  This method determines the source lines that correspond to any
     *  uncovered ranges or branches.
     */
    void findSourceForUncovered( void );

    /*!
     *  This method returns the total number of branches always taken
     *  for all analyzed symbols.
     *
     *  @return Returns the total number of branches always taken
     */
    uint32_t getNumberBranchesAlwaysTaken( void ) const;

    /*!
     *  This method returns the total number of branches found for
     *  all analyzed symbols.
     *
     *  @return Returns the total number of branches found
     */
    uint32_t getNumberBranchesFound( void ) const;

    /*!
     *  This method returns the total number of branches never taken
     *  for all analyzed symbols.
     *
     *  @return Returns the total number of branches never taken
     */
    uint32_t getNumberBranchesNeverTaken( void ) const;

    /*!
     *  This method returns the total number of uncovered ranges
     *  for all analyzed symbols.
     *
     *  @return Returns the total number of uncovered ranges
     */
    uint32_t getNumberUncoveredRanges( void ) const;

    /*!
     *  This method returns an indication of whether or not the specified
     *  symbol is a symbol to analyze.
     *
     *  @return Returns TRUE if the specified symbol is a symbol to analyze
     *   and FALSE otherwise.
     */
    bool isDesired (
      const std::string& symbolName
    ) const;

    /*!
     *  This method creates the set of symbols to analyze from the symbols
     *  listed in the specified file.
     */
    void load(
      const char* const symbolsFile
    );

    /*!
     *  This method merges the coverage information from the source
     *  coverage map into the unified coverage map for the specified symbol.
     *
     *  @param[in] symbolName specifies the symbol associated with the
     *             destination coverage map
     *  @param[in] sourceCoverageMap specifies the source coverage map
     */
    void mergeCoverageMap(
      const std::string&           symbolName,
      const CoverageMapBase* const sourceCoverageMap
    );

    /*!
     *  This method preprocesses each symbol's coverage map to mark nop
     *  and branch information.
     */
    void preprocess( void );

    /*!
     *  This member contains the statistics kept on each symbol.
     */    
    Statistics stats;

  private:

    /*!
     *  This method uses the specified executable file to determine the
     *  source lines for the elements in the specified ranges.
     */
    void determineSourceLines(
      CoverageRanges* const theRanges,
      ExecutableInfo* const theExecutable
    );

  };
}

#endif
