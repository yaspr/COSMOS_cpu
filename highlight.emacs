(defvar cosmos-mode-syntax-table nil "Syntax table for `cosmos-mode'.")

;; Handling keywords and symbols
(setq cosmos-highlights
      '(
	("fms\\|fmsv\\|loadc\\|storc" 						  . font-lock-keyword-face)
	("and\\|or\\|xor\\|not\\|shl\\shr\\|rol\\|ror\\|pcnt\\|lmb\\|inv\\|\\rev" . font-lock-keyword-face)
        ("addv\\|subv\\|mulv\\|divv\\|modv\\|fmav\\|redv"			  . font-lock-keyword-face)
	("andv\\|orv\\|xorv\\|notv\\|shlv\\shrv\\|rolv\\|rorv"			  . font-lock-keyword-face)
	("add\\|sub\\|mul\\|div\\|mod\\|inc\\|dec\\|fma"			  . font-lock-keyword-face)
	("loadw\\|storw\\|loadb\\|storb\\|loadv\\|storv"			  . font-lock-keyword-face)
        ("cmp\\|jmp\\|jeq\\|jne\\|jgt\\|jlt\\|jge\\|jle"			  . font-lock-keyword-face)
	("movw\\|movb\\|mov"							  . font-lock-keyword-face)
       	("prntw\\|prntb\\|prnts\\|prntx"                                          . font-lock-keyword-face)
       	("tsc\\|hlt\\|rand\\|ff"						  . font-lock-keyword-face)
	("@" 									  . font-lock-type-face)
	("0\\|1\\|2\\|3\\|4\\|5\\|6\\|7\\|8\\|9"				  . font-lock-constant-face)))

;; Handling comments # ...
(setq cosmos-mode-syntax-table
      	(let ( (synTable1 (make-syntax-table)))
        (modify-syntax-entry ?# "<" synTable1)
        (modify-syntax-entry ?\n ">" synTable1)
        synTable1))

;; Defining cosmos-mode
(define-derived-mode cosmos-mode fundamental-mode "cosmos"
  "major mode for editing cosmos language code."
  (setq font-lock-defaults '(cosmos-highlights))
  (set-syntax-table cosmos-mode-syntax-table))