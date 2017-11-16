# SO HeatSim - Group 48

## TODO list

### Exercise 4
#### Basic Requirements
* [x] `fichS`: file that contains the matrix to resume processing.
* [x] `fichS`: if file doesn't exist, start from scratch (as usual).
* [x] Have `periodoS` as number of iterations between saves. if `periodoS == 0`, don't save at all.
* [x] `simul()` should proceed even while saving to file is happening. To pull this off, make use of `fork()`.
* [x] Whenever heatSim ends successfully, remove `fichS`.

#### Advanced Requirements
* [x] Write matrix between `periodoS` to file in a temporary file before renaming it back to the official name.
* [x] Make use of `waitpid(pid, &wstatus, WNOHANG)` before launching a new child process to avoid concurrent writing to file..
* [ ] Have `periodoS` an interval in seconds between saves, instead of iterations.
* [x] Save matrix to file when a CTRL-C (or _SIGINT_) has been issued.
* [ ] _SIGINT_ is signaled to a child process as well, so it should be redefined in that child.

For signal-related content, check the following pages from the book:
- 3.5.3 _Signals_ - 97
- 4.4.7 Implementação dos _Signals_ - 172
- 6.7 Monitores - 274

#### Miscellaneous
* [ ] Use `WNOHANG` in `waitpid()` and fix all dependent issues.
* [ ] Experiment (read notes from paper).
* [ ] Check with teacher.
