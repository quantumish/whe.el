(add-to-list 'load-path "~/projects/wheelmacs/")
(require 'wheel)
;; (load-file "~/projects/wheelmacs/whe.el")
(wheel-init)
;; (wheel-def "C-x g"
;;   :starboard hydra-go-to-file/body
;;   :port magit-status
;;   :none vterm)


;; (defun test () (interactive)
;; 	   (while t
;; 		 (let ((diff (wheel-axis 0)))
;; 		   (cond ((> diff 0) (parrot-rotate-next-word-at-point))
;; 				 ((< diff 0) (parrot-rotate-prev-word-at-point)))		   
;; 		   )))
