#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <list>
#include <fstream>

#include "app_common.h"
#include "CoverageFactory.h"
#include "CoverageMap.h"
#include "DesiredSymbols.h"
#include "ExecutableInfo.h"
#include "Explanations.h"
#include "ObjdumpProcessor.h"
#include "ReportsBase.h"
#include "TargetFactory.h"
#include "GcovData.h"
#include "SymbolSetReader.h"
#include "SymbolSet.h"

#include "rld-process.h"

/*
 *  Variables to control general behavior
 */
const char*                          coverageFileExtension = NULL;
std::list<std::string>               coverageFileNames;
int                                  coverageExtensionLength = 0;
Coverage::CoverageFormats_t          coverageFormat;
Coverage::CoverageReaderBase*        coverageReader = NULL;
char*                                executable = NULL;
const char*                          executableExtension = NULL;
int                                  executableExtensionLength = 0;
std::list<Coverage::ExecutableInfo*> executablesToAnalyze;
const char*                          explanations = NULL;
char*                                progname;
const char*                          symbolsFile = NULL;
const char*                          symbolSetFile = NULL;
const char*	                         gcnosFileName = NULL;
char                                 gcnoFileName[FILE_NAME_LENGTH];
char                                 gcdaFileName[FILE_NAME_LENGTH];
char                                 gcovBashCommand[256];
const char*                          target = NULL;
const char*                          format = NULL;
FILE*                                gcnosFile = NULL;
Gcov::GcovData*                      gcovFile;

/*
 *  Print program usage message
 */
void usage()
{
  std::cout << "Usage: " << progname
            << " [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -1 EXECUTABLE coverage1 ... coverageN" << std::endl
            << "--OR--" << std::endl
            << "Usage: " << progname
            << " [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -e EXE_EXTENSION -c COVERAGEFILE_EXTENSION EXECUTABLE1 ... EXECUTABLE2" << std::endl
            << std::endl
            << " -v                  - verbose output" << std::endl
            << " -T TARGET           - architecture target name" << std::endl
            << " -f FORMAT           - simulator format " << std::endl
            << "(RTEMS, QEMU, TSIM or Skyeye)" << std::endl
            << " -E EXPLANATIONS     - file of explanations" << std::endl
            << " -s SYMBOLS_FILE     - symbols of interest" << std::endl
            << " -S SYMBOL_SET_FILE  - path to symbol_sets.cfg" << std::endl
            << " -1 EXECUTABLE       - executable to get symbols from"
            << std::endl
            << " -e EXE_EXTENSION    - suffix for executables" << std::endl
            << " -c COVERAGEFILE_EXT - coverage file suffix" << std::endl
            << " -g GCNOS_LIST       - list of *.gcno files" << std::endl
            << " -p PROJECT_NAME     - name of the project" << std::endl
            << " -O Output_Directory - output directory default=." << std::endl
            << " -d debug            - disable cleaning of tempfiles."
            << std::endl
            << std::endl;
}

static void
fatal_signal( int signum )
{
  signal( signum, SIG_DFL );

  rld::process::temporaries_clean_up();

  /*
   * Get the same signal again, this time not handled, so its normal effect
   * occurs.
   */
  kill( getpid(), signum );
}

static void
setup_signals( void )
{
  if ( signal( SIGINT, SIG_IGN ) != SIG_IGN )
    signal( SIGINT, fatal_signal );
#ifdef SIGHUP
  if ( signal( SIGHUP, SIG_IGN ) != SIG_IGN )
    signal( SIGHUP, fatal_signal );
#endif
  if ( signal( SIGTERM, SIG_IGN ) != SIG_IGN )
    signal( SIGTERM, fatal_signal );
#ifdef SIGPIPE
  if ( signal( SIGPIPE, SIG_IGN ) != SIG_IGN )
    signal( SIGPIPE, fatal_signal );
#endif
#ifdef SIGCHLD
  signal( SIGCHLD, SIG_DFL );
#endif
}

int main(
  int    argc,
  char** argv
)
{
  int ec = 0;
  setup_signals ();

  try
  {
    std::list<std::string>::iterator               citr;
    std::string                                    coverageFileName;
    std::string                                    notFound;
    std::list<Coverage::ExecutableInfo*>::iterator eitr;
    Coverage::ExecutableInfo*                      executableInfo = NULL;
    int                                            i;
    int                                            opt;
    const char*                                    singleExecutable = NULL;
    std::string                                    option;
    rld::process::tempfile                         objdumpFile( ".dmp" );
    rld::process::tempfile                         err( ".err" );
    rld::process::tempfile                         syms( ".syms" );
    bool                                           debug = false;

   /*
    * Process command line options.
    */
    progname = argv[0];

    while ( (opt = getopt( argc, argv, "C:1:L:e:c:g:E:f:s:S:T:O:p:v:d" )) != -1 ) {
      switch( opt ) {
        case '1': singleExecutable      = optarg; break;
        case 'L': dynamicLibrary        = optarg; break;
        case 'e': executableExtension   = optarg; break;
        case 'c': coverageFileExtension = optarg; break;
        case 'g': gcnosFileName         = optarg; break;
        case 'E': explanations          = optarg; break;
        case 'f': format                = optarg; break;
        case 's': symbolsFile           = optarg; break;
        case 'S': symbolSetFile         = optarg; break;
        case 'T': target                = optarg; break;
        case 'O': outputDirectory       = optarg; break;
        case 'v': Verbose               = true;   break;
        case 'p': projectName           = optarg; break;
        case 'd': debug                 = true;   break;
        default: /* '?' */
          usage();
          exit( -1 );
      }
    }

    try
    {
     /*
      * Validate inputs.
      */

     /*
      * Target name must be set.
      */
      if ( !target ) {
        option = "target -T";
        throw option;
      }

     /*
      * Validate simulator format.
      */
      if ( !format ) {
        option = "format -f";
        throw option;
      }

     /*
      * Validate that we have a symbols of interest file.
      */
      if ( !symbolSetFile ) {
        option = "symbol set file -S";
        throw option;
      }

     /*
      * Has path to explanations.txt been specified.
      */
      if ( !explanations ) {
        option = "explanations -E";
        throw option;
      }

     /*
      * Has coverage file extension been specified.
      */
      if ( !coverageFileExtension ) {
        option = "coverage extension -c";
        throw option;
      }

     /*
      * Has executable extension been specified.
      */
      if ( !executableExtension ) {
        option = "executable extension -e";
        throw option;
      }

     /*
      * Check for project name.
      */
      if ( !projectName ) {
        option = "project name -p";
        throw option;
      }
    }
    catch( std::string option )
    {
      std::cout << "error missing option: " + option << std::endl;
      usage();
      throw;
    }

   /*
    * If a single executable was specified, process the remaining
    * arguments as coverage file names.
    */
    if ( singleExecutable ) {

     /*
      * Ensure that the executable is readable.
      */
      if ( !FileIsReadable( singleExecutable ) ) {
        std::string oneExec = singleExecutable;
        throw rld::error( "cannot read", "executable: " + oneExec );
      }
      else {
        for ( i=optind; i < argc; i++ ) {

         /*
          * Ensure that the coverage file is readable.
          */
          if ( !FileIsReadable( argv[i] ) ) {
            std::string coverageFile = argv[i];
            throw rld::error( "cannot read", "coverage file: " + coverageFile );
          }
          else
            coverageFileNames.push_back( argv[i] );
        }

       /*
        * If there was at least one coverage file, create the
        * executable information.
        */
        if ( !coverageFileNames.empty() ) {
          if ( dynamicLibrary )
            executableInfo = new Coverage::ExecutableInfo(
              singleExecutable, dynamicLibrary
            );
          else
            executableInfo = new Coverage::ExecutableInfo( singleExecutable );
          executablesToAnalyze.push_back( executableInfo );
        }
      }
    }

   /*
    * If not invoked with a single executable, process the remaining
    * arguments as executables and derive the coverage file names.
    */
    else {
      try
      {
        for ( i = optind; i < argc; i++ ) {

         /*
          * Ensure that the executable is readable.
          */
          if ( !FileIsReadable( argv[i] ) ) {
            std::string executable = argv[i];
            throw executable;
          }
          else {
            coverageFileName = argv[i];
            coverageFileName.replace(
              coverageFileName.length() - executableExtensionLength,
              executableExtensionLength,
              coverageFileExtension
            );
            if ( !FileIsReadable( coverageFileName.c_str() ) ) {
              throw coverageFileName;
            }
            else {
              executableInfo = new Coverage::ExecutableInfo( argv[i] );
              executablesToAnalyze.push_back( executableInfo );
              coverageFileNames.push_back( coverageFileName );
            }
          }
        }
      }
      catch( std::string fileName )
      {
        std::cout << fileName + "is not readable." << std::endl;
      }
    }

   /*
    * Ensure that there is at least one executable to process.
    */
    if ( executablesToAnalyze.empty() ) {
      throw rld::error( "no executables to analyse",
                         "covoar cmd line arguments" );
    }
    if ( Verbose ) {
      if ( singleExecutable )
        std::cout << "Processing single executable and multiple coverage files"
                  << std::endl;
      else
        std::cout << "Processing multiple executable/coverage file pairs"
                  << std::endl;
      std::cout << "Coverage Format : " << format << std::endl
                << "Target          : " << target << std::endl
                << std::endl;
  #if 1
     /*
      * Process each executable/coverage file pair.
      */
      eitr = executablesToAnalyze.begin();
      for ( citr = coverageFileNames.begin();
            citr != coverageFileNames.end();
            citr++ ) {
      std::cout << "Coverage file " << *citr
                << "for executable " << (*eitr)->getFileName() << std::endl;
      if ( !singleExecutable )
        eitr++;
      }
  #endif
    }

   /*
    * Create data to support analysis.
    */

   /*
    * Create data based on target.
    */
    TargetInfo = Target::TargetFactory( target );

   /*
    * Create the set of desired symbols.
    */
    SymbolsToAnalyze = new Coverage::DesiredSymbols();

   /*
    *Load symbols from specified symbolsFile
    */
    if ( symbolsFile ) {
        SymbolsToAnalyze->load( symbolsFile );
    }

   /*
    *Read symbol configuration file and load needed symbols
    */
    if ( symbolSetFile ) {
      std::cout << "Reading configuration symbol set file: " << symbolSetFile
                << std::endl;
      Symbols::SymbolSetReader ssr;
      std::vector<Symbols::SymbolSet> symbolSets = ssr.readSetFile( symbolSetFile );
      Symbols::SymbolSet& set = symbolSets[0];
      std::cout << "Generating symbol file for " + set.getName() << std::endl;
      set.generateSymbolFile( syms, target );
      SymbolsToAnalyze->load( syms.name().c_str() );
    }
    if ( Verbose )
      std::cout << "Analyzing " + SymbolsToAnalyze->set.size()
                << "symbols" << std::endl;

   /*
    * Create explanations.
    */
    AllExplanations = new Coverage::Explanations();
    if ( explanations )
      AllExplanations->load( explanations );

   /*
    * Create coverage map reader.
    */
    coverageReader = Coverage::CreateCoverageReader( coverageFormat );
    if ( !coverageReader ) {
      throw rld::error( "Unable to create coverage file reader",
                        "CreateCoverageReader" );
    }

   /*
    * Create the objdump processor.
    */
    objdumpProcessor = new Coverage::ObjdumpProcessor();

   /*
    * Prepare each executable for analysis.
    */
    for ( eitr = executablesToAnalyze.begin();
          eitr != executablesToAnalyze.end();
          eitr++ ) {
      if ( Verbose )
        std::cout << "Extracting information from " + (*eitr)->getFileName()
                  << std::endl;
     /*
      * If a dynamic library was specified, determine the load address.
      */
      if ( dynamicLibrary )
        (*eitr)->setLoadAddress(
          objdumpProcessor->determineLoadAddress( *eitr )
        );
     /*
      * Load the objdump for the symbols in this executable.
      */
      objdumpProcessor->load( *eitr, objdumpFile, err );
    }

   /*
    * Analyze the coverage data.
    */

   /*
    * Process each executable/coverage file pair.
    */
    eitr = executablesToAnalyze.begin();
    for ( citr = coverageFileNames.begin();
          citr != coverageFileNames.end();
          citr++ ) {

      if ( Verbose )
        std::cout << "Processing coverage file " << *citr
                  << "for executable " << (*eitr)->getFileName() << std::endl;
     /*
      * Process its coverage file.
      */
      coverageReader->processFile( (*citr).c_str(), *eitr );

     /*
      * Merge each symbols coverage map into a unified coverage map.
      */
      (*eitr)->mergeCoverage();

      if ( !singleExecutable )
        eitr++;
    }

   /*
    * Do necessary preprocessing of uncovered ranges and branches
    */
    if ( Verbose )
      std::cout << "Preprocess uncovered ranges and branches" << std::endl;
    SymbolsToAnalyze->preprocess();

   /*
    * Generate Gcov reports
    */
/*  if (Verbose)
      fprintf( stderr, "Generating Gcov reports...\n");
    gcnosFile = fopen ( gcnosFileName , "r" );

    if ( !gcnosFile ) {
      fprintf( stderr, "Unable to open %s\n", gcnosFileName );
    }
    else {
      while ( fscanf( gcnosFile, "%s", inputBuffer ) != EOF) {
        gcovFile = new Gcov::GcovData();
        strcpy( gcnoFileName, inputBuffer );

        if ( Verbose )
      fprintf( stderr, "Processing file: %s\n", gcnoFileName );

        if ( gcovFile->readGcnoFile( gcnoFileName ) ) {
          // Those need to be in this order
          gcovFile->processCounters();
          gcovFile->writeReportFile();
          gcovFile->writeGcdaFile();
          gcovFile->writeGcovFile();
        }

        delete gcovFile;
      }
    fclose( gcnosFile );
    }
*/
   /*
    * Determine the uncovered ranges and branches.
    */
    if ( Verbose )
      std::cout << "Computing uncovered ranges and branches" << std::endl;
    SymbolsToAnalyze->computeUncovered();

   /*
    * Calculate remainder of statistics.
    */
    if ( Verbose )
      std::cout << "Calculate statistics" << std::endl;
    SymbolsToAnalyze->calculateStatistics();

   /*
    * Look up the source lines for any uncovered ranges and branches.
    */
    if ( Verbose )
        std::cout << "Looking up source lines for uncovered ranges and branches"
                  << std::endl;
    SymbolsToAnalyze->findSourceForUncovered();

   /*
    * Report the coverage data.
    */
    if ( Verbose )
      std::cout << "Generate Reports" << std::endl;
    Coverage::GenerateReports();

   /*
    * Write explanations that were not found.
    */
    if ( explanations ) {
      notFound = outputDirectory;
      notFound += "/";
      notFound += "ExplanationsNotFound.txt";

      if ( Verbose )
        std::cout << "Writing Not Found Report (" + notFound
                  << ")" << std::endl;
      AllExplanations->writeNotFound( notFound.c_str() );
    }

   /*
    * Leaves tempfiles around if debug flag (-d) is enabled.
    */
    if ( debug ) {
      rld::process::set_keep_temporary_files();
      objdumpFile.override( "objdump_file" );
      objdumpFile.keep();
      err.override( "objdump_exec_log" );
      err.keep();
      syms.override( "symbols_list" );
      syms.keep();
    }
  }
  catch( rld::error re )
  {
    std::cerr << "error: "
              << re.where << ": " << re.what
              << std::endl;
    ec = 1;
  }
  catch( ... )
  {
    /*
     * Helps to know if this happens.
     */
    std::cout << "error: unhandled exception" << std::endl;
    ec = 2;
  }
  return ec;
}
