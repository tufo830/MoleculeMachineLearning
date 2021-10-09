#pragma once
#include<vector>
#include<math.h>
#include<float.h>

#include "PolicyValueNet.h"
#include "ThreadPool.h"


using std::vector;

class TreeNode {
public:
	friend class MCTS;
	TreeNode(double prior, TreeNode* parent, int action_n) :p(prior), parent(parent), children(action_n, nullptr) {
		q = visit_n = vloss=0;
		is_leaf = 1;
		next_action = -1;
	}
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
		this->children = node.children;
		this->parent = node.parent;
		this->visit_n.store(node.visit_n.load());
		this->vloss.store(node.vloss.load());
		this->is_leaf = node.is_leaf;
	}

	double get_value(double cpuct,double cvloss,int visit_sum) {
		int visit_n = this->visit_n.load();
		double virtual_loss = cvloss * this->vloss.load();
		if(virtual_loss!=0)
		printf("virtual loss is %f\n", virtual_loss);
		double u = (cpuct * this->p * sqrt(visit_sum)) / (visit_n + 1);
		if (visit_n <= 0) {
			return u - virtual_loss;
		}
		return u + this->q  - virtual_loss / visit_n;
	}
	void expend(const vector<double>& action_prior) {
		if (!this->is_leaf) {
			printf("the same path\n");
			return;
		}
		int action_size = this->children.size();
		std::unique_lock<std::mutex> lock;
		for (int i = 0; i < action_size; i++) {
			if (action_prior[i] > 0.0003) {
				this->children[i] = new TreeNode(action_prior[i], this, action_size);
			}
		}
		this->is_leaf = 0;
	}
	int select(double cpuct,double cvloss) {
		double maxv = -30000000000;
		int action = -1;
		int visit_sum = this->visit_n.load();
		for (int i = 0; i < children.size(); i++) {
			if (children[i]) {
				double value = children[i]->get_value(cpuct,cvloss,visit_sum);
				if (value > maxv) {
					maxv = value;
					action = i;
				}
			}
		}
		this->children[action]->vloss++;
		return action;
	}

	void backup(double value) {
		if (this->parent) {
			this->parent->backup(-value);
		}
		//printf("backing %f\n", value);
		visit_n = this->visit_n.load();
		this->visit_n++;
		this->vloss--;
		{
			std::lock_guard<std::mutex> lock(this->lock);
			this->q += (value - this->q) / (visit_n+1);
		}
	}

	~TreeNode() {
		for (int i = 0; i < children.size(); i++) {
			if (i != next_action && children[i]) {
				delete children[i];
			}
		}
	}
private:
	std::mutex lock;
	std::atomic<int> visit_n;
	std::atomic<int> vloss;
	int is_leaf, next_action;
	double q, p; //p is the prior probability,q is the mean value
	TreeNode* parent;
	vector<TreeNode*> children;
};

class MCTS {
public:
	MCTS(int n, int simulation,double cpuct,double cvloss) :net(n),pool(4) {
		this->root = new TreeNode(1.0, nullptr, n * n);
		this->simulation = simulation;
		this->cpuct = cpuct;
		this->n = n;
		this->cvloss = cvloss;
	}
	vector<int> getVisitNumber(const Board board) {
		vector<int> visit_num(n * n, 0);
	std:vector<std::future<void>> futures;
		for (int i = 0; i < this->simulation; i++) {
			Board board_copy = board;
			std::future<void> fu=this->pool.commit(std::bind(&MCTS::simulate, this, board_copy));
			futures.emplace_back(std::move(fu));
		}
		for (int i = 0; i < simulation; i++) {
			futures[i].wait();
		}
		for (int i = 0; i < n * n; i++) {
			if (root->children[i]) {
				visit_num[i] = root->children[i]->visit_n.load();
			}
		}
		return visit_num;
	}
	void simulate(Board board) {
		//printf("start simulation\n");
		TreeNode* node = this->root;
		while (!node->is_leaf) {
			int action = node->select(cpuct,cvloss);
			//printf("choosen action %d\n", action);
			board.do_move(action);
			node = node->children[action];
		}
		//printf("start get expand\n");
		int status = board.end();
		double value = -100;
		if (status >= 0) {
			//printf("the game end,winner is %d", status);
			if (status == board.current_player) value = 1;
			else value = -1;
		}
		else if (status == -1) {
			//printf("the game end,tie\n");
			value = 0.0;
		}
		else {
			vector<double> probability = net.getprob(board, value);
			//board.display();
			//printf("\nprobility\n");
			//for (int i = 0; i < n; i++) {
			//	for (int j = 0; j < n; j++)
			//		printf("%.5f ", probability[i * n + j]);
			//	printf("\n");
			//}
			//printf("%.3f\n", value);
			node->expend(probability);
		}
		//printf("start backup\n");
		node->backup(-value);
	}
	void update(int action) {
		if (action == -1) return;
		root->next_action = action;
		if (this->root->children[action]) {
			TreeNode* t = root;
			root = root->children[action];
			delete t;
			root->parent = nullptr;
		}
	}
	~MCTS() {
		if (root) {
			delete root;
			root = nullptr;
		}
	}

private:
	PolicyValueNet net;
	double cpuct,cvloss;
	int simulation, n;
	TreeNode* root;
	ThreadPool pool;
};