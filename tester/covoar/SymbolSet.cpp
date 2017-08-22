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

namespace Symbols
{
  SymbolSet::SymbolSet ()
  {
  }

  SymbolSet::~SymbolSet ()
  {
  }

  std::string SymbolSet::parseNmOutputLine (std::string line)
  {
    std::string symbol = "";
    if (line.find ("FUNC|") != std::string::npos)
    {
      symbol = line.substr (0, line.find ('|'));
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

  void SymbolSet::parseNmOutput (std::ifstream& nm_out, const std::string& lib)
  {
    std::string line, symbol;
    while (getline (nm_out, line))
    {
      symbol = parseNmOutputLine (line);
      if (symbol.length () > 0)
      {
        symbols.push_back (symbol + " " + getLibname (lib));
      }
    }
  }

  void SymbolSet::generateSymbolFile (rld::process::tempfile& filePath,
                                      std::string target)
  {
    std::string nm_output = "nm.out";
    std::string nm_error = "nm.err";
    std::string libFiles;

    for (std::string lib : libraries)
    {
      try
      {
        auto status = rld::process::execute (target + "-nm",
                std::vector<std::string> { target + "-nm", "--format=sysv",
                        lib }, nm_output, nm_error);
        if (status.type != rld::process::status::normal or status.code != 0)
        {
          std::cout << "ERROR: nm returned " << status.code << std::endl;
          std::cout << "For details see " << nm_error
                    << " file." << std::endl;
          std::remove (nm_output.c_str ());
          return;
        }
      }
      catch (rld::error& err)
      {
        std::cout << "Error while running nm for " + lib << std::endl;
        std::cout << err.what << " in " << err.where << std::endl;
        return;
      }

      std::ifstream nm_out (nm_output);
      try
      {
        parseNmOutput (nm_out, lib);
      }
      catch (std::exception& e)
      {
        std::cout << "ERROR while parsing nm output: " << e.what ()
                  << std::endl;
      }
      nm_out.close ();
    }

    std::remove (nm_output.c_str ());
    std::remove (nm_error.c_str ());

    std::ofstream outputFile (filePath.name ());
    for (std::string symbol : symbols)
      outputFile << symbol << std::endl;
    outputFile.close ();
  }
}
