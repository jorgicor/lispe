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

(define map0 (lambda (fn p)
    (if (null? p)
      '()
      (cons (fn (car p)) (map0 fn (cdr p))))))

(define let (special (varlist . rest)
    (cons (cons 'lambda (cons (map0 car varlist) rest))
	  (map0 cadr varlist))))

; append lists on a new list except that the last argument is shared
; if no arguments return nil
(define (append . args)
  (define (append-acc head node cur r)
    (cond [(null? cur)
	   (cond [(null? r) head]
		 [(null? (cdr r)) (set-cdr! node (car r)) head]
		 [else (append-acc head node (car r) (cdr r))])]
	  [else (set-cdr! node (cons (car cur) '()))
		(append-acc head (cdr node) (cdr cur) r)]))
  (if (null? args)
    args
    (let ([tmp-node (cons '() '())])
      (cdr (append-acc tmp-node tmp-node (car args) (cdr args))))))

; reverse list p
(define (reverse p)
  (define (rev head p)
    (if (null? p)
      head
      (rev (cons (car p) head) (cdr p))))
  (if (null? p)
    '()
    (rev (cons (car p) '()) (cdr p))))

(define (list-tail p n)
  (if (zero? n)
    p
    (list-tail (cdr p) (- n 1))))

(define (list-ref p n)
  (car (list-tail p n)))

(define (list-set! p n elem)
  (set-car! (list-tail p n) elem))

(define (member elem p . compare)
  (define (memb elem p cmp)
    (cond [(null? p) #f]
	  [(cmp elem (car p)) p]
	  [else (memb elem (cdr p) cmp)]))
  (if (null? compare)
    (memb elem p equal? p)
    (memb elem p (car compare))))

(define (memq elem p)
  (member elem p eq?))

(define (memv elem p)
  (member elem p eqv?))

(define (list-copy p)
  (define (copy head node p)
    (if (null? p)
      head
      (begin (set-cdr! node (cons (car p) '()))
	     (copy head (cdr node) (cdr p)))))
  (if (null? p)
    '()
    (let ([head (cons (car p) '())])
      (copy head head (cdr p)))))

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
    (if (null? p)
      n
      (len-iter (+ n 1) (cdr p))))
  (len-iter 0 p))

(define (map fn first . rest)
  (define (map-lists fn pp)
    (if (null? (car pp))
      '()
      (cons (apply fn (map0 car pp))
	    (map-lists fn (map0 cdr pp)))))
  (map-lists fn (cons first rest)))

(define (abs n)
  (if (< n 0)
    (- n)
    n))

(define (fact n)
  (if (= n 0)
    1
    (* n (fact (- n 1)))))

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
  (if (= n 0)
    (stream-car s)
    (stream-ref (stream-cdr s) (- n 1))))

(define (stream-map proc . ss)
  (if (stream-null? (car ss))
    the-empty-stream
    (cons-stream (apply proc (map stream-car ss))
		 (apply stream-map (cons proc (map stream-cdr ss))))))

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
  (if (> low hi)
    the-empty-stream
    (cons-stream low (stream-interval (+ low 1) hi))))

(define (is-sqrt n)
  (define (is-sqrt-iter i n)
    (let ([mul (* i i)])
      (cond [(> mul n) #f]
	    [(= mul n) #t]
	    [else (is-sqrt-iter (+ i 1) n)])))
  (is-sqrt-iter 1 n))

(define (count n)
  (define (count-iter i n)
    (if (>= i n)
      n
      (count-iter (+ i 1) n)))
  (count-iter 0 n))

(define (count-apply n)
  (define (count-iter i n)
    (if (>= i n)
      n
      (apply count-iter (list (+ i 1) n))))
  (count-iter 0 n))

(define d (stream-filter is-sqrt integers))

