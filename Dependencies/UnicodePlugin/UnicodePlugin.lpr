library UnicodePlugin;

{$mode objfpc}{$H+}

uses
  SysUtils,
  Classes,
  lazUtf8;

function IsUtf8(const AString: PAnsiChar): Boolean; stdcall; export;
begin
  Result := False;
  if Assigned(AString) then
     Result := FindInvalidUTF8Codepoint(AString, StrLen(AString)) = -1;
end;

function AnsiToUTF8(const ASource: PAnsiChar; ADest: PAnsiChar): Integer;
  stdcall; export;
var Utf8Str: Utf8String;
begin
  Result := -1;
  Utf8Str := WinCPToUTF8(RawByteString(ASource));
  Result := Length(Utf8Str);
  if Result = 0 then Exit;
  if not Assigned(ADest) then
    Result := Length(Utf8Str) + 1
  else
    StrLCopy(ADest, PAnsiChar(Utf8Str), Result);
end;

function UTF8ToAnsi(const ASource: PAnsiChar; ADest: PAnsiChar;
  const ATestOnInvalid: Boolean): Integer;
  stdcall; export;
var AnsiStr: RawByteString;
begin
  Result := -1;
  if ATestOnInvalid and
    (FindInvalidUTF8Codepoint(ASource, Length(ASource)) <> -1) then Exit;
  AnsiStr := UTF8ToWinCP(RawByteString(ASource));
  Result := Length(AnsiStr);
  if Result = 0 then Exit;
  if not Assigned(ADest) then
    Result := Length(AnsiStr) + 1
  else
    StrLCopy(ADest, PAnsiChar(AnsiStr), Result);
end;

function UTF8ToWide(const ASource: PAnsiChar; ADest: PWideChar;
  const ATestOnInvalid: Boolean): Integer;
  stdcall; export;
var WideStr: WideString;
begin
  Result := -1;
  if ATestOnInvalid and
    (FindInvalidUTF8Codepoint(ASource, Length(ASource)) <> -1) then Exit;
  WideStr := UTF8ToUTF16(RawByteString(ASource));
  Result := Length(WideStr);
  if Result = 0 then Exit;
  if not Assigned(ADest) then
    Result := Length(WideStr) + 1
  else
    StrLCopy(ADest, PWideChar(WideStr), Result);
end;

function WideToUTF8(const ASource: PWideChar; ADest: PAnsiChar): Integer;
  stdcall; export;
var Utf8Str: Utf8String;
begin
  Result := -1;
  Utf8Str := UTF16ToUTF8(WideString(ASource));
  Result := Length(Utf8Str);
  if Result = 0 then Exit;
  if not Assigned(ADest) then
    Result := Length(Utf8Str) + 1
  else
    StrLCopy(ADest, PAnsiChar(Utf8Str), Result);
end;

{ exports }

exports
  UTF8ToAnsi index 1,
  AnsiToUTF8 index 2,
  UTF8ToWide index 3,
  WideToUTF8 index 4,
  IsUtf8 index 5;

{$R *.res}

begin
end.

