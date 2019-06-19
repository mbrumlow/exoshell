
;; (let ((quicklisp-init (merge-pathnames "quicklisp/setup.lisp"
;;                                        (user-homedir-pathname))))
;;   (when (probe-file quicklisp-init)
;;     (load quicklisp-init)))

(let ((quicklisp-init "quicklisp/setup.lisp"))
  (when (probe-file quicklisp-init)
    (load quicklisp-init)))


(ql:quickload "swank")
(require 'swank)


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


(defun start-swank () 
  (ql:quickload "swank")
  (require 'swank)

  (setf swank:*use-dedicated-output-stream* nil)
  (setq slime-net-coding-system 'utf-8-unix)
  (setf swank:*communication-style* :SPAWN)
  (swank:create-server :port 4006 :dont-close t )
  (loop (sleep 1)))

;; (defun start-swank () 

;;   )


;; (swank:create-server :port 4006  :dont-close t :coding-system "utf-8-unix")

;(loop (sleep 1))
