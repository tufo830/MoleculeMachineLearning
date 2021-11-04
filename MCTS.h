#pragma once
#include<vector>
#include<math.h>
#include<float.h>

#include "PolicyValueNet.h"
#include "ThreadPool.h"


class TreeNode {
public:
	friend class MCTS;
	TreeNode(double prior, TreeNode* parent, int move) 
		:p(prior), 
		parent(parent),
		move(move) ,
		q(0),
		visit_n(0),
		vloss(0),
		is_leaf(1),
		next_move(-1){}

	TreeNode(const TreeNode& node) {
		this->copy(node);
	}
	TreeNode& operator = (const TreeNode& node) {
		this->copy(node);
		return *this;
	}
	void copy(const TreeNode& node) {
		this->p = node.p;
		this->q = node.q;
		this->move = node.move;
		this->next_move = node.next_move;
		this->children = node.children;
		this->parent = node.parent;
		this->visit_n.store(node.visit_n.load());
		this->vloss.store(node.vloss.load());
		this->is_leaf = node.is_leaf;
	}

	double get_value(double cpuct,double cvloss,int visit_sum) {
		int visit_n = this->visit_n.load();
		double virtual_loss = cvloss * this->vloss.load();
		double u = (cpuct * this->p * sqrt(visit_sum)) / (visit_n + 1);
		return u + this->q  - virtual_loss / (visit_n+1);
	}
	void expend(const std::vector<double>& action_prior) {
		std::lock_guard<std::mutex> lock(this->lock);
		if (!this->is_leaf) 
			return;

		for (int i = 0; i < action_prior.size(); i++) {
			if (action_prior[i] > 0.003) {
				this->children.emplace_back(new TreeNode(action_prior[i], this, i));
			}
		}
		this->is_leaf = 0;
	}
	int select(double cpuct,double cvloss) {
		double maxv = -DBL_MAX;
		int action = -1;
		int visit_sum = this->visit_n.load();
		for (int i = 0; i < children.size(); i++) {
			double value = children[i]->get_value(cpuct,cvloss,visit_sum);
			if (value > maxv) 
			{
				maxv = value;
				action = i;
			}
		}
		this->children[action]->vloss++;
		return action;
	}

	void backup(double value) {
		visit_n = this->visit_n.load();
		this->visit_n++;
		this->vloss--;

		{
			std::lock_guard<std::mutex> lock(this->lock);
			this->q += (value - this->q) / (visit_n+1);
		}
		if (this->parent)
			this->parent->backup(-value);
	}

	~TreeNode() {
		for (int i = 0; i < children.size(); i++) {
			if (i != next_move)
				delete children[i]; 
		}
	}
private:
	std::mutex lock;
	std::atomic<int> visit_n;
	std::atomic<int> vloss;
	int is_leaf, move,next_move;
	double q, p; 
	TreeNode* parent;
	std::vector<TreeNode*> children;
};

class MCTS {
public:
	MCTS(int n, int simulation,double cpuct,double cvloss)
		:n(n),
		simulation(simulation),
		cpuct(cpuct),
		cvloss(cvloss),
		root(new TreeNode(1.0,nullptr,-1)),
		network(n,4),
		pool(4){}

	std::vector<int> getVisitNumber(const Board& board) {
		std::vector<int> visit_num(n * n, 0);
	    std::vector<std::future<void>> futures;

		for (int i = 0; i < this->simulation; i++) {
			Board board_copy = board;
			std::future<void> future=this->pool.commit(std::bind(&MCTS::simulate, this, board_copy));
			futures.emplace_back(std::move(future));
		}
		for (int i = 0; i < simulation; i++) 
			futures[i].wait();
		
		for (int i = 0; i < root->children.size(); i++) 
			visit_num[root->children[i]->move] = root->children[i]->visit_n.load();

		return visit_num;
	}

	void update(int move) {
		for (int i = 0; i < root->children.size(); i++) {
			if (root->children[i]->move == move) {
                 root->next_move = i;
				 TreeNode* t = root;
				 root = root->children[i];
				 delete t;
				 root->parent = nullptr;
				 return;
			}
		}
		//printf("the next move is not in chilren\n");
		delete root;
		root = new TreeNode(1.0, nullptr, move);
	}
	~MCTS() {
		if (root) {
			delete root;
			root = nullptr;
		}
	}

private:
	void simulate(Board& board) {
		TreeNode* node = this->root;

		while (!node->is_leaf) {
			int action = node->select(cpuct, cvloss);
			board.do_move(node->children[action]->move);
			node = node->children[action];
		}

		int status = board.end();
		double value=0;
		if (status >= 0) {
			//end
			value = (status == board.current_player) ? 1 : -1;
		}
		else if (status == -1) {
			//tie
			value = 0.0;
		}
		else {
			//not end,expand
			std::vector<double>probability = network.commit(board).get();
			value = probability.back();
			probability.pop_back();
			node->expend(probability);
		}
		node->backup(-value);
	}


	PolicyValueNet network;
	double cpuct,cvloss;
	int simulation, n;
	TreeNode* root;
	ThreadPool pool;
};