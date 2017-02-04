; Library procedures

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
  (cond [(eq? b #t) #t]
	[(eq? b #f) #t]
	[else #f]))

(define begin (special rest
    (list (cons 'lambda (cons '() rest)))))

(define (caar p) (car (car p)))
(define (cadr p) (car (cdr p)))
(define (cdar p) (cdr (car p)))
(define (cddr p) (cdr (cdr p)))

(define (append first . rest)
  (define (append-acc head node cur r)
    (cond [(null? cur)
	   (cond [(null? r) head]
		 [(null? (cdr r)) (set-cdr! node (car r)) head]
		 [else (append-acc head node (car r) (cdr r))])]
	  [else (set-cdr! node (cons (car cur) '()))
		(append-acc head (cdr node) (cdr cur) r)]))
  (let ([tmp-node (cons '() '())])
    (cdr (append-acc tmp-node tmp-node first rest))))

(define (max first . rest)
  (define (maxn m p)
    (cond [(null? p) m]
	  [(> m (car p)) (maxn m (cdr p))]
	  [else (maxn (car p) (cdr p))]))
  (maxn first rest))

(define (min first . rest)
  (define (minn m p)
    (cond [(null? p) m]
	  [(< m (car p)) (minn m (cdr p))]
	  [else (minn (car p) (cdr p))]))
  (minn first rest))

(define (list? p)
  (cond [(null? p) #t]
	[(pair? p) (list? (cdr p))]
	[else #f]))

(define list (lambda rest rest))

(define (length p)
  (define (len-iter n p)
    (cond [(null? p) n]
	  [else (len-iter (+ n 1) (cdr p))]))
  (len-iter 0 p))

(define map0 (lambda (fn p)
    (cond [(null? p) '()]
          [else (cons (fn (car p)) (map0 fn (cdr p)))])))

(define let (special (varlist . rest)
    (cons (cons 'lambda (cons (map0 car varlist) rest))
	  (map0 cadr varlist))))

(define (map fn first . rest)
  (define (map-lists fn pp)
    (cond [(null? (car pp)) '()]
	  [else (cons (apply fn (map0 car pp))
		      (map-lists fn (map0 cdr pp)))]))
  (map-lists fn (cons first rest)))

(define (abs n)
  (cond [(< n 0) (- n)]
	[else n]))

(define (fact n)
  (cond [(= n 0) 1]
	[else (* n (fact (- n 1)))]))

(define delay (special (expr)
  (list 'lambda '() expr)))

(define (force delayed-expr)
  (delayed-expr))

(define cons-stream (special (a b)
  (list 'cons a (list 'delay b))))

(define the-empty-stream '())
(define stream-null? null?)

(define (stream-car stream) (car stream))
(define (stream-cdr stream) (force (cdr stream)))

(define (stream-ref s n)
  (cond [(= n 0) (stream-car s)]
	[else (stream-ref (stream-cdr s) (- n 1))]))

(define (stream-map proc . ss)
  (cond ((stream-null? (car ss)) the-empty-stream)
	(else (cons-stream (apply proc (map stream-car ss))
			   (apply stream-map
				  (cons proc (map stream-cdr ss)))))))

(define (stream-filter pred stream)
  (cond [(stream-null? stream) the-empty-stream]
	[(pred (stream-car stream))
	 (cons-stream (stream-car stream)
		      (stream-filter pred (stream-cdr stream)))]
	[else (stream-filter pred (stream-cdr stream))]))

(define (add-streams s1 s2)
  (stream-map + s1 s2))

(define ones (cons-stream 1 ones))

(define integers (cons-stream 1 (add-streams ones integers)))

(define (stream-interval low hi)
  (cond [(> low hi) the-empty-stream]
	[else (cons-stream low (stream-interval (+ low 1) hi))]))

(define (is-sqrt n)
  (define (is-sqrt-iter i n)
    (define mul (* i i))
    (cond [(> mul n) #f] 
	  [(= mul n) #t]
	  [else (is-sqrt-iter (+ i 1) n)]))
  (is-sqrt-iter 1 n))

(define (count n)
  (define (count-iter i n)
    (cond [(>= i n) n]
	  [else (count-iter (+ i 1) n)]))
  (count-iter 0 n))

(define (count-apply n)
  (define (count-iter i n)
    (cond [(>= i n) n]
	  [else (apply count-iter (list (+ i 1) n))]))
  (count-iter 0 n))

(define d (stream-filter is-sqrt integers))

