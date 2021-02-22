fib <- function(n) {
  res <- c(1, 1, numeric(n-2))
  for (i in 3:length(res)) {
    res[i] <- res[i-1] + res[i-2]
  }
  return(res)
}

print(system.time(fib(50)))