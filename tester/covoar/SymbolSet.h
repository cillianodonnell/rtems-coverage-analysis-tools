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

#ifndef SYMBOLSET_H_
#define SYMBOLSET_H_

#include <string>
#include <vector>

#include "rld-process.h"

namespace Symbols
{
  class SymbolSet
  {
  public:
    SymbolSet ();
    virtual ~SymbolSet ();

    const std::string getName () const
    {
      return name;
    }

    void setName (const std::string& name)
    {
      this->name = name;
    }

    const std::vector<std::string> getLibraries () const
    {
      return libraries;
    }

    void addLibrary (std::string libraryPath)
    {
      libraries.push_back (libraryPath);
    }

    void generateSymbolFile (rld::process::tempfile& filePath,
                             std::string target);

  private:
    std::string name;
    std::vector<std::string> libraries;
    std::vector<std::string> symbols;

    std::string parseElfDataLine (std::string line);
    std::string getLibname (std::string libPath);
    void parseElfData (rld::process::tempfile& elfData,
                       const std::string& lib);
  };
}

#endif /* SYMBOLSET_H_ */
