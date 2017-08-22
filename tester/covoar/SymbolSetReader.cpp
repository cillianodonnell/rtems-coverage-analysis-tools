/*
 * Copyright 2014 Krzysztof Miesowicz (krzysztof.miesowicz@gmail.com)
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
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "SymbolSetReader.h"
#include "iostream"
#include "fstream"

namespace Symbols
{
  SymbolSetReader::SymbolSetReader ()
  {
  }

  SymbolSetReader::~SymbolSetReader ()
  {
  }

  std::vector<SymbolSet> SymbolSetReader::readSetFile (string filepath)
  {
    std::vector<SymbolSet> setList {};
    ifstream file (filepath);
    string line;
    while (getline (file, line))
    {
      if (line.find ("symbolset:") != std::string::npos)
      {
        setList.emplace_back ();
      }
      else if (line.length () > 0)
      {
        auto pair = parseLine (line);

        if (pair.first == "name")
        {
          if (setList.empty ())
          {
            setList.emplace_back ();
          }
          setList.rbegin ()->setName (pair.second);
          continue;
        }
        if (pair.first == "lib")
        {
          setList.rbegin ()->addLibrary (pair.second);
          continue;
        }
        std::cout << "Invalid entry in configuration file : " << line
                  << endl;
        break;
      }
    }
    file.close ();
    return setList;
  }

  std::pair<string, string> SymbolSetReader::parseLine (string line)
  {
    size_t delimeterPosition = line.find ('=');

    if (delimeterPosition != std::string::npos)
    {
      string key = line.substr (0, delimeterPosition);
      string value = line.substr (delimeterPosition + 1);
      return {trim (key), trim (value)};
    }
    return {"",""};
  }

  string& SymbolSetReader::trim (string& str)
  {
   /*
    * trim trailing spaces
    */
    size_t endpos = str.find_last_not_of (" \t\n\r");
    if (string::npos != endpos)
    {
      str = str.substr (0, endpos + 1);
    }

   /*
    * trim leading spaces
    */
    size_t startpos = str.find_first_not_of (" \t\n\r");
    if (string::npos != startpos)
    {
      str = str.substr (startpos);
    }
    return str;
  }
}
