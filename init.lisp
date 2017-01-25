(setq else t)

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

(setq begin (special (&rest)
      (eval (cons (cons 'lambda (cons nil &rest)) nil))))

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

(setq let (special (varlist &rest)
    (eval (cons
	    (cons 'lambda (cons (map car varlist) &rest))
	    (map cadr varlist)))))

(setq unev-let (special (varlist &rest)
    (cons
	    (cons 'lambda (cons (map car varlist) &rest))
	    (map cadr varlist))))

(setq and (special (&rest)
    (eval (unev-let ((andlis nil))
      (setq andlis (lambda (q)
	(cond ((null q) t)
	      ((null (car q)) nil)
	      ((null (cdr q)) (car q))
	      (t (andlis (cdr q))))))
      (andlis &rest)))))

(setq or (special (&rest)
    (eval (unev-let ((orlis nil))
      (setq orlis (lambda (q)
	(cond ((null q) nil)
	      ((not (null (car q))) (car q))
	      (t (orlis (cdr q))))))
      (orlis &rest)))))

(setq <= (lambda (a &rest) 
    (eval (unev-let ((lesseq nil))
        (setq lesseq (lambda (a q)
            (cond ((null q) t)
		  ((eq a (car q)) (lesseq (car q) (cdr q)))
		  ((< a (car q)) (lesseq (car q) (cdr q)))
		  (t nil))))
        (lesseq a &rest)))))

(setq >= (lambda (a &rest) 
    (eval (unev-let ((greatereq nil))
        (setq greatereq (lambda (a q)
            (cond ((null q) t)
		  ((eq a (car q)) (greatereq (car q) (cdr q)))
		  ((> a (car q)) (greatereq (car q) (cdr q)))
		  (t nil))))
        (greatereq a &rest)))))

(setq inc (special (var)
    (eval (list 'setq var (list '+ 1 var)))))

(setq set (lambda (x y)
    (eval (list 'setq x 'y))))

(setq push (special (a p)
   (set p (eval (list 'cons a p)))))

(setq fact (lambda (n)
    (cond ((eq n 0) 1)
	  (t (* n (fact (- n 1)))))))

(setq make-counter (lambda (value)
    (closure () (setq value (+ value 1)))))

(setq make-balance (lambda (balance)
    (closure (amount) (setq balance (- balance amount)))))

(setq sqrt2 (lambda (x)
    (let ((abs nil)
	  (average nil)
	  (square nil)
	  (good-enough? nil)
	  (improve nil)
	  (sqrt-iter nil))
      (setq abs (lambda (n) (cond ((< n 0) (- n)) (t n))))
      (setq average (lambda (x y) (/ (+ x y) 2)))
      (setq square (lambda (n) (* n n)))
      (setq good-enough? (lambda (guess)
			    (< (abs (- (square guess) x)) (/ 1 100))))
      (setq improve (lambda (guess) (average guess (/ x guess))))
      (setq sqrt-iter (lambda (guess)
			 (cond ((good-enough? guess) guess)
			       (t (sqrt-iter (improve guess))))))
      (sqrt-iter 1))))

(setq sqrt (lambda (x)
    ((lambda (abs average square good-enough? improve sqr-iter)
       (setq abs (lambda (n) (cond ((< n 0) (- n)) (t n))))
       (setq average (lambda (x y) (/ (+ x y) 2)))
       (setq square (lambda (n) (* n n)))
       (setq good-enough? (lambda (guess)
			    (< (abs (- (square guess) x)) (/ 1 100))))
       (setq improve (lambda (guess) (average guess (/ x guess))))
       (setq sqrt-iter (lambda (guess)
			 (cond ((good-enough? guess) guess)
			       (t (sqrt-iter (improve guess))))))
       (sqrt-iter 1)) nil nil nil nil nil nil)))

(setq define (special (a &rest)
      (eval
          (cond ((symbolp a) (list 'setq a (car &rest)))
	        (t (list 'setq (car a)
			 (cons 'lambda (cons (cdr a) &rest))))))))

(define (cons2 x y)
  (let ((dispatch nil))
    (setq dispatch (closure (m)
			    (cond ((eq m 0) x)
				  ((eq m 1) y)
				  (t 'error))))
    dispatch))

(setq nil! (special (x)
    (eval (list 'setq x nil))))

