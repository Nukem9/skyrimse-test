# SkyrimSETest

<p align="center">
A collection of modifications, enhancements, and reverse engineered resources for Creation Kit Skyrim: Special Edition
</p>

# Fixed or added
:white_check_mark: Fast file loading  
:white_check_mark: Ability to remove hints on the progress bar  
:white_check_mark: Multiple UI Fix View Menu section and etc  
:white_check_mark: Create master files and open them as plugins  
:white_check_mark: Opening plugins as master files if they are in dependencies  
:white_check_mark: Filtering by active forms and cell
:white_check_mark: Filtering mods in the Data dialog
:white_check_mark: Fix compact plugin
:white_check_mark: Added Unicode support
Many other patches and fixes.

# Unicode
**ONLY SURROGATE**: Support for English and native languages without special characters.
For understanding, the application uses the ancient **ANSI** type string representation. The idea is to feed the text already in your native encoding. When loading .esp, .esl, .esm files all strings are translated from UTF-8 to WinCP, and when saved back. WinCP is the current encoding in your operating system.  
**IMPORTANT**:  
In **Win10**, in the language settings, there is now an option to work with UTF-8, you need to turn it off otherwise there will be only "?".

# License
![MIT License](https://camo.githubusercontent.com/20666e1b72ed1ea8f0a7c1d1e0ea35769a7c24f879ecc27ac16641b46f225a01/68747470733a2f2f6f70656e736f757263652e6f72672f74726164656d61726b732f6f70656e736f757263652f4f53492d417070726f7665642d4c6963656e73652d313030783133372e706e67)

MIT License

Copyright (c) 2021 Nukem9 <email:Nukem@outlook.com>
Copyright (c) 2022-2023 Perchik71 <email:timencevaleksej@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.