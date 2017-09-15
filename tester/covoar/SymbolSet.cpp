/*
 * Copyright 2014 Krzysztof Miesowicz  (krzysztof.miesowicz@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <fstream>
#include <cstdio>

#include "SymbolSet.h"

#include "rld.h"
#include "rld-process.h"
#include "rld-symbols.h"
#include "rld-files.h"

namespace Symbols
{
  SymbolSet::SymbolSet ()
  {
  }

  SymbolSet::~SymbolSet ()
  {
  }

  std::string SymbolSet::parseElfDataLine (std::string line)
  {
    std::string symbol = "";
    int funcStartPos = 64;
    if (line.find ("STT_FUNC") != std::string::npos)
    {
      symbol = line.substr (funcStartPos);
      symbol = symbol.substr (0, symbol.find (' '));
    }
    return symbol;
  }

  std::string SymbolSet::getLibname (std::string libPath)
  {
    std::string libname = "", base = "", temp;
    size_t pos = libPath.find_last_of ('/');
    if (pos != std::string::npos)
    {
      temp = libPath.substr (0, pos);
      libname = libPath.substr (pos + 1);
    }
    pos = temp.find_last_of ('/');
    if (pos != std::string::npos)
    {
      base = temp.substr (pos + 1);
    }
    return base + "/" + libname;
  }

  void SymbolSet::parseElfData (rld::process::tempfile& elfData,
                                const std::string& lib)
  {
    std::string line, symbol;
    elfData.open( true );
    while ( true )
    {
      elfData.read_line (line);
      if ( line.empty() ) break;
      symbol = parseElfDataLine (line);
      if (symbol.length () > 0)
      {
        symbols.push_back (symbol + " " + getLibname (lib));
      }
    }
  }

  void SymbolSet::generateSymbolFile (rld::process::tempfile& filePath,
                                      std::string target)
  {
    rld::files::cache   kernel;
    rld::symbols::table symbolsTable;

    for (std::string lib : libraries)
    {
      /*
       * Load the symbols from the kernel.
       */
      try
      {
        /*
         * Load the kernel ELF file symbol table.
         */
        kernel.open ();
        kernel.add (lib);
        kernel.load_symbols (symbolsTable, true);

        /*
         * Create a symbols file.
         */
        std::ofstream mout;
        mout.open (filePath.name().c_str());
        if (!mout.is_open ())
          throw rld::error ("map file open failed", "map");
        mout << "RTEMS Kernel Symbols Map" << std::endl
             << " kernel: " << lib << std::endl
             << std::endl;
        rld::symbols::output (mout, symbolsTable);
        mout.close ();
      }
      catch (...)
      {
        kernel.close ();
        throw;
      }

      kernel.close ();

      try
      {
        parseElfData (filePath, lib);
      }
      catch (std::exception& e)
      {
        std::cout << "ERROR while parsing symbols output: " << e.what ()
                  << std::endl;
      }
      filePath.close ();
    }

    std::ofstream outputFile (filePath.name ());
    for (std::string symbol : symbols)
      outputFile << symbol << std::endl;
    outputFile.close ();
  }
}
