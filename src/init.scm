(define else #t)

(define (null? p)
  (eq? p '()))

(define (zero? n)
  (eqv? n 0))

(define (positive? n)
  (> n 0))

(define (negative? n)
  (< n 0))

(define (not b)
  (eq? b #f))

(define (boolean? b)
  (cond ((eq? b #t) #t)
	((eq? b #f) #t)
	(else #f)))

(define begin (special (&rest)
    (list (cons 'lambda (cons '() &rest)))))

(define (caar p) (car (car p)))
(define (cadr p) (car (cdr p)))
(define (cdar p) (cdr (car p)))
(define (cddr p) (cdr (cdr p)))

(define (max first &rest)
  (define (maxn m p)
    (cond ((null? p) m)
	  ((> m (car p)) (maxn m (cdr p)))
	  (else (maxn (car p) (cdr p)))))
  (maxn first &rest))

(define (min first &rest)
  (define (minn m p)
    (cond ((null? p) m)
	  ((< m (car p)) (minn m (cdr p)))
	  (else (minn (car p) (cdr p)))))
  (minn first &rest))

(define (list? p)
  (cond ((null? p) #t)
	((pair? p) (list? (cdr p)))
	(else #f)))

(define (list &rest) &rest)

(define (length p)
  (define (len-iter n p)
    (cond ((null? p) n)
	  (else (len-iter (+ n 1) (cdr p)))))
  (len-iter 0 p))

(define map0 (lambda (fn p)
    (cond ((null? p) '())
           (else (cons (fn (car p)) (map0 fn (cdr p)))))))

(define let (special (varlist &rest)
    (cons (cons 'lambda (cons (map0 car varlist) &rest))
	  (map0 cadr varlist))))

(define (map fn first &rest)
  (define (map-lists fn pp)
    (cond ((null? (car pp)) '())
	  (else (cons (apply fn (map0 car pp)) (map-lists fn (map0 cdr pp))))))
  (map-lists fn (cons first &rest)))

