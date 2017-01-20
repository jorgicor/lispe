(setq listp (lambda (p)
    (cond ((null p) t)
	  ((consp p) t)
	  (t nil))))

(setq zerop (lambda (n)
    (cond ((eq n 0) t)
	  (t nil))))

(setq not null)

(setq booleanp (lambda (b)
    (cond ((null b) t)
	  ((eq b t) t)
	  (t nil))))

(setq map (lambda (fn p)
    (cond ((null p) nil)
	  (t (cons (fn (car p)) (map fn (cdr p)))))))

(setq if (special (a b c)
    (cond ((eval a) (eval b))
	  (t (eval c)))))

(setq caar (lambda (p) (car (car p))))
(setq cadr (lambda (p) (car (cdr p))))
(setq cdar (lambda (p) (cdr (car p))))
(setq cddr (lambda (p) (cdr (cdr p))))

(setq last (lambda (p)
    (cond ((null p) nil)
	  ((null (cdr p)) (car p))
	  ((not (consp (cdr p))) p)
	  (t (last (cdr p))))))

(setq lastcons (lambda (p)
    (cond ((consp p)
	   	(cond ((null (cdr p)) p)
		      (t (lastcons (cdr p))))
	   (t nil)))))

(setq nconc (lambda (p q)
    (setcdr (lastcons p) q)))

(setq append (lambda (p q)
    (cond ((null p)
	   	(cond ((null q) nil)
		       (t (cons (car q) (append p (cdr q)))))
           )
	   (t (cons (car p) (append (cdr p) q))))))

(setq and (special (p)
    ((lambda andlis (q)
        (cond ((null q) t)
	      ((null (car q)) nil)
	      (t (andlis (cdr q)))))
     (evlis p))))

(setq set (lambda (x y)
    (eval (list 'setq x 'y))))

(setq push (special (a p)
   (set p (eval (list 'cons a p)))))

(setq fact (lambda (n)
    (cond ((eq n 0) 1)
	  (t (* n (fact (- n 1)))))))

