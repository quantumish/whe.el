(defun wheel-starboard? ()
  "Are we headed starboard?"
  (> (wheel-axis 0) 0))

(defun wheel-port? ()
  "Are we headed port?"
  (< (wheel-axis 0) 0))

(cl-defmacro wheel-def (keyb &key starboard port none)
  `(general-def
	 ,keyb (lambda () (interactive)
	   (cond ((wheel-starboard?) (,starboard))
			 ((wheel-port?) (,port))
			 (t (,none))))))
