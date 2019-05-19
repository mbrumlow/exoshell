
(load "quicklisp.lisp")

(let ((quicklisp-init (merge-pathnames "quicklisp/setup.lisp"
                                       (user-homedir-pathname))))
  (when (probe-file quicklisp-init)
    (load quicklisp-init)))

(ql:quickload "cl-interpol")

(named-readtables:in-readtable :interpol-syntax)


(defun start-shell ()
  (named-readtables:in-readtable :interpol-syntax)
  (si::top-level))

;; (defun outershell-start()
;;   (si::top-level) 
;;   (outershell-end))


;; (defun outershell ()
;;    (mp:process-run-function 'outershell #'outershell-start))
