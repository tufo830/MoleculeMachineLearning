# BetaCat0
This is a simple Gomoku game with deep reinforcement learning and Monte Carlo Tree Search(like Alphagozero) implemented by C++，
so you can download the release and play it on your computer,

The repo used model from https://github.com/initial-h/AlphaZero_Gomoku_MPI, sincerely grateful for it.  

The model was trained with python and tensorflow, I needed to build tensorflow from source code and frozened the model in order to load it in C++ code.

multi-thread was was applied,and the batchsize was set to 4 to accelerate search and inference.


