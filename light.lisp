(setf grammar
      '((start (mu yellow) prod start)
        (prod (pred down) loops (pred down)       (mu red) (pred (not down)) R)
        (prod (pred down) loops (pred (not down)) (mu blu) (pred (not down)) B)
        (loops (pred down) (mu red) (pred down) (mu blu) loops)
        (loops )
        (prod (pred (not down)))
        (R (mu strong-red) (mu strong-yellow) prod (mu strong-red))
        (B (mu strong-blu) (mu strong-yellow) prod (mu strong-blu))
        )
      )
