(defconstant +colon+ :colon)
(defconstant +period+ :period)
(defconstant +identifier+ :identifier)
(defconstant +instruction+ :instruction)
(defconstant +register+ :register)
(defconstant +number+ :number)

(defstruct token
  type
  str)

(defun create-token (type str)
  (make-token :type type :str (copy-seq str)))

(defun expect (expected ptr)
  (char= expected (char ptr 0)))

(defun expect-condition (condition ptr)
  (funcall condition (char ptr 0)))

(defun tokenize (line)
  (let ((tokens '())
        (i 0))
    (loop
      while (< i (length line))
      for ch = (char line i)
      do
        (cond
          ((or (char= ch #\Space) (char= ch #\Tab))
           (incf i))

          ((char= ch #\:)
           (push (create-token +colon+ ":") tokens)
           (incf i))

          ((char= ch #\.)
           (push (create-token +period+ ".") tokens)
           (incf i))

          ((char= ch #\R)
           (let ((next-i (1+ i)))
             (if (and (< next-i (length line))
                      (digit-char-p (char line next-i)))
                 (let ((start next-i))
                   (loop while (and (< next-i (length line))
                                    (digit-char-p (char line next-i)))
                         do (incf next-i))
                   (push (create-token +register+
                                       (subseq line i next-i)) tokens)
                   (setf i next-i))
                 (error "Expected register after 'R'"))))

          ((alpha-char-p ch)
           (let ((start i))
             (loop while (and (< i (length line))
                              (alpha-char-p (char line i)))
                   do (incf i))
             (push (create-token +instruction+
                                 (subseq line start i)) tokens)))

          ((digit-char-p ch)
           (let ((start i))
             (loop while (and (< i (length line))
                              (digit-char-p (char line i)))
                   do (incf i))
             (push (create-token +number+
                                 (subseq line start i)) tokens)))

          (t (incf i))))
    (nreverse tokens)))
