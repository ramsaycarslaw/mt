// test the log module

for (var i = 1; i <= 100; i = i + 1) {
  log.Progress(i, 100);
  sleep(0.2);
}
