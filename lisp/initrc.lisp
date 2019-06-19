;; User exoshell  home 
(setq exoshell-home (merge-pathnames ".exoshell.d/"
                                       (user-homedir-pathname)))

;; User init file. 
(setq user-init-file (merge-pathnames ".exoshell.d/init.lisp"
                                       (user-homedir-pathname)))

(defun term-get-lines (n)
  (let ((*ret* (list)))
    (dotimes (i n)
      (let ((line (term-get-last-line i)))
        (when line
            (setq *ret* (append *ret* (list (term-get-last-line i)))))))
  *ret*))

(defun default-shell()
  (si::top-level))

(defun exoshell-start ()
  (handler-bind ((error (function invoke-debugger)))
    (loop
    (with-simple-restart
     (default-shell "Restart default-shell") (default-shell)))))

(defun process-run-function (process-name process-function &rest args)
  (let ((process (mp:make-process process-name)))
    (apply #'mp:process-preset process function args)
    (mp:process-enable process)))

(defun quicklisp-load ()
  (let ((quicklisp-init (merge-pathnames "quicklisp/setup.lisp" exoshell-home)))
    (if (probe-file quicklisp-init)
        (load quicklisp-init)
        (progn
          (load (merge-pathnames "quicklisp.lisp" exoshell-dist))
          (load (merge-pathnames "quicklisp-util.lisp" exoshell-dist))
          (quicklisp-setup)))))

;; Load user init if it exist. 
(when (probe-file user-init-file)
  (load user-init-file)))

