const struct msg msgs_de[] = {
  { "Division by zero", "Division durch Null" }, // Argos
  // re G/en [division with zero]
  // re A/en [Division by Zero]

  { "Floating point exception", "Fließkomma-Ausnahme" }, // Google
  // re A/en [Flow-compartment take-off]
  // re G/en [Flow-point exception]

  { "Overflow", "Überlauf" }, // Argos
  // re G/en [Overflow]
  // re A/en [Override]

  { "Subscript out of range", "Index außerhalb des Bereichs" }, // Google
  // re A/en [Index outside the range]
  // re G/en [Index outside the area]

  { "Icode buffer full", "Icode-Puffer voll" }, // Google
  // re A/en [Icode buffer full]
  // re G/en [Icode buffer full]

  { "GOSUB too many nested", "zu viele geschachtelte GOSUB" }, // manual

  { "RETURN stack underflow", "RETURN-Stapelunterlauf" }, // Argos edited
  // re G/en [Return stacking]
  // re A/en [RETURN Stack underrun]

  { "Argument stack overflow", "Argument-Stapelüberlauf" }, // Google edited
  // re A/en [Stack overrunning]
  // re G/en [Argument stack overflow]

  { "Undefined argument", "Undefiniertes Argument" }, // Argos
  // re G/en [Undefined argument]
  // re A/en [Undefined argument]

  { "FOR too many nested", "zu viele geschachtelte FOR" }, // manual

  { "NEXT without FOR", "NEXT ohne FOR" }, // Argos
  // re G/en [Next without for]
  // re A/en [NEXT without FOR]

  { "FOR without variable", "FOR ohne Variable" }, // Google edited
  // re A/en [For without variable.]
  // re G/en [For without variable.]

  { "FOR without TO", "FOR ohne TO" }, // manual

  { "LET without variable", "LET ohne Variable" }, // Argos
  // re G/en [LET without variable]
  // re A/en [LET without variable]

  { "IF without condition", "IF ohne Bedingung" }, // Argos
  // re G/en [IF without condition]
  // re A/en [IF without condition]

  { "IF stack overflow", "Überlauf des IF-Stapels" }, // Argos edited
  // re G/en [Overflow of the stack]
  // re A/en [Overflow of the stack]

  { "IF stack underflow", "IF-Stapelunterlauf" }, // Argos
  // re G/en [IF stacking]
  // re A/en [IF-stamp underrun]

  { "Undefined line number", "Undefinierte Zeilennummer" }, // Argos
  // re G/en [Undefined line number]
  // re A/en [Undefined line number]

  { "\'(\' or \')\' expected", "\'(\' oder \')\' erwartet" }, // Argos edited
  // re G/en [\ '(\' or \ '' '' 'Expected]
  // re A/en [\'(\' or \'''' expected]

  { "\'=\' expected", "\'=\' erwartet" }, // Argos
  // re G/en [\ = \ 'expects]
  // re A/en [\'=\' expected]

  { "Cannot use system command", "Systembefehl kann nicht verwendet werden" }, // Google
  // re A/en [System command cannot be used]
  // re G/en [System command can not be used]

  { "Illegal value", "Illegaler Wert" }, // Google
  // re A/en [Illegal value]
  // re G/en [Illegal value]

  { "Out of range value", "Wert außerhalb des Bereichs" }, // Google edited
  // re A/en [Outside the area]
  // re G/en [Outside the range]

  { "Program not found", "Programm nicht gefunden" }, // Argos
  // re G/en [Program not found]
  // re A/en [Page not found]

  { "Syntax error", "Syntax-Fehler" }, // Argos
  // re G/en [Syntax error]
  // re A/en [Syntax error]

  { "Internal error", "Interner Fehler" }, // Argos
  // re G/en [Internal error]
  // re A/en [Internal error]

  { "Break", "Unterbrechung" }, // Google
  // re A/en [Interruption]
  // re G/en [Interruption]

  { "Line too long", "Zeile zu lang" }, // Google
  // re A/en [Line to long]
  // re G/en [Line too long]

  { "File write error", "Datei-Schreibfehler" }, // manual

  { "File read error", "Datei-Lesefehler" }, // manual

  { "Cannot use GPIO function", "GPIO-Funktion kann nicht verwendet werden" }, // Google
  // re A/en [GPIO function cannot be used]
  // re G/en [GPIO function can not be used]

  { "Too long path", "Zu langer Pfad" }, // Google edited
  // re A/en [To long path]
  // re G/en [Too long path]

  { "File open error", "Fehler beim Datei-Öffnen" }, // manual

  { "File not open", "Datei nicht geöffnet" }, // Google
  // re A/en [File not opened]
  // re G/en [File not open]

  { "SD I/O error", "SD I/O-Fehler" }, // Argos
  // re G/en [SD I / O error]
  // re A/en [SD I/O error]

  { "Bad file name", "Schlechter Dateiname" }, // Google
  // re A/en [Bad file name]
  // re G/en [Bad filename]

  { "Not supported", "Nicht unterstützt" }, // Argos
  // re G/en [Unsupported]
  // re A/en [Not supported]

  { "Out of video memory", "Videospeicher voll" }, // manual

  { "Out of memory", "Speicher voll" }, // manual

  { "Can't continue", "Kann nicht weitermachen" }, // Argos
  // re G/en [Can not go on]
  // re A/en [Can't go on]

  { "Undefined array", "Undefiniertes Array" }, // Argos
  // re G/en [Undefined array]
  // re A/en [Undefined array]

  { "Undefined procedure", "Undefinierte Prozedur" }, // manual

  { "Undefined label", "Undefiniertes Label" }, // Argos
  // re G/en [Undefined label]
  // re A/en [Undefined label]

  { "Duplicate procedure", "Prozedur mit gleichem Namen existiert" }, // manual

  { "Duplicate label", "Label mit gleichem Namen existiert" }, // manual

  { "Out of data", "Daten zu Ende" }, // manual

  { "I/O error", "I/O-Fehler" }, // Argos
  // re G/en [I / O error]
  // re A/en [I/O error]


  { "Type mismatch", "Typ-Mismatch" }, // Google edited
  // re A/en [TYP-MISMATCH.]
  // re G/en [Type MISMATCH.]

  { "Local variable outside procedure", "Lokale Variable außerhalb einer Prozedur" }, // manual

  { "Empty list", "Leere Liste" }, // Argos
  // re G/en [Blank list]
  // re A/en [Empty list]

  { "ENDIF not found", "ENDIF nicht gefunden" }, // Argos
  // re G/en [Endif not found]
  // re A/en [ENDIF not found]

  { "Not enough arguments", "Nicht genug Argumente" }, // Argos
  // re G/en [Not enough arguments]
  // re A/en [Not enough arguments]

  { "File format error", "Dateiformatfehler" }, // Argos edited
  // re G/en [File format error]
  // re A/en [File format Error]

  { "USING format error", "USING-Formatfehler" }, // manual
  // re A/en [Use the format error.]
  // re G/en [Use the format error.]

  { "Network error", "Netzwerkfehler" }, // Argos
  // re G/en [Network error]
  // re A/en [Network errors]

  { "Not a directory", "Kein Verzeichnis" }, // Google
  // re A/en [No list]
  // re G/en [No directory]

  { "WHILE without WEND", "WHILE ohne WEND" }, // Google edited
  // re A/en [While without cry.]
  // re G/en [While without turn.]

  { "System stack overflow", "Systemstapelüberlauf" }, // Google
  // re A/en [System stack overflow]
  // re G/en [System stack overflow]

  { "File seek error", "Dateisuchfehler" }, // manual

  { "Failed to init synth", "Synth konnte nicht initialisiert werden" }, // manual

  { "Unknown command", "Unbekannter Befehl" }, // Argos
  // re G/en [unknown command]
  // re A/en [Unknown command]

  { "PROC without CALL/FN", "PROC ohne CALL/FN" }, // Argos
  // re G/en [Proc without call / fn]
  // re A/en [PROC without CALL/FN]

  { "CHAIN failed", "CHAIN gescheitert" }, // Argos edited
  // re G/en [Chain has failed]
  // re A/en [CHAIN has failed]

  { "Path too long", "Pfad zu lang" }, // Google
  // re A/en [Path to long]
  // re G/en [Path]

  { "Undefined symbol", "Undefiniertes Symbol" }, // Argos
  // re G/en [Undefined symbol]
  // re A/en [Undefined symbol]

  { "Compiler error", "Compiler-Fehler" }, // Google
  // re A/en [Compiler error]
  // re G/en [Compiler error]

  { NULL, NULL },
};
