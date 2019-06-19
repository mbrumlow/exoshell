
(defun quicklisp-install ()
  (quicklisp-quickstart:install :path (merge-pathnames "quicklisp/" exoshell-home)))

(defun quicklisp-setup ()
  (let ((quicklisp-init (merge-pathnames "quicklisp/setup.lisp" exoshell-home)))
    (if (probe-file quicklisp-init)
        (load quicklisp-init)
        (progn
          (quicklisp-quickstart:install :path (merge-pathnames "quicklisp/" exoshell-home))
          (load quicklisp-init)))))

