#include <future>
#include<queue>
#include "Board.h"

#include <tensorflow/core/platform/env.h>
#include <tensorflow/core/public/session.h>


class PolicyValueNet
{
public:

	using return_type = std::vector<double>;

	PolicyValueNet(int n = 15,int batchsize=4, std::string model_path = "./frozen_model.pb")
		:n(n),batchsize(batchsize),running(true),model_path(model_path)
	{
		status = tensorflow::NewSession(tensorflow::SessionOptions(), &session);
		tensorflow::GraphDef graph_def;
		status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), model_path, &graph_def);
		status = session->Create(graph_def);
		if (!status.ok()) {
			std::cout << status.ToString() << std::endl;
		}
		this->loop = std::make_unique<std::thread>([this]() {
			while (this->running) {
				this->infer();
			}
			});
	}
	std::future<return_type> commit(const Board& board) {
		std::promise<return_type> promise;
		std::future<return_type> future = promise.get_future();
		{
			std::lock_guard<std::mutex> lock(this->lock);
			tasks.emplace(std::make_pair(board.move_seq,std::move(promise)));
		}
		cv.notify_all();
		return future;
	}

	~PolicyValueNet() {
		this->running = false;
		this->loop->join();
		session->Close();
		session = nullptr;
	}
private:

    //combine move sequence and the result,the probability and value saved in one vector
	using task_type = std::pair<std::vector<int>, std::promise<return_type>>;

	void infer() {
		std::vector < std::vector<int>> states;
		std::vector<std::promise<return_type>> promises;
		bool timeout = false;
		while (!timeout && states.size() < this->batchsize) {
			using namespace std::chrono_literals;
			std::unique_lock<std::mutex> lock(this->lock);
			if (cv.wait_for(lock, 1ms, [this]() {return this->tasks.size() > 0; })) {
				auto task = std::move(tasks.front());
				states.emplace_back(task.first);
				promises.emplace_back(std::move(task.second));
				this->tasks.pop();
			}
			else {
				timeout = true;
			}
		}
		if (states.size() == 0) 
			return; 

		int bs = states.size();
		tensorflow::Tensor state(tensorflow::DT_FLOAT, tensorflow::TensorShape({ bs,9,15,15 }));
		auto input = state.tensor<float, 4>();

		for (int b = 0; b < bs; b++) {
			int move_num = states[b].size();
			for (int p = 0; p < 8; p++) {
				for (int h = 0; h < n; h++)
					for (int w = 0; w < n; w++)
						input(b, p, h, w) = 0.0;
				if (p % 2 == 1) {
					for (int i = move_num - 2; i >= 0; i -= 2) {
						int h = states[b][i] / n, w = states[b][i] % n;
						input(b, p, h, w) = 1.0;
					}
				}
				else {
					for (int i = move_num - 1; i >= 0; i -= 2) {
						int h = states[b][i] / n, w = states[b][i] % n;
						input(b, p, h, w) = 1.0;
					}
				}
			}

			for (int h = 0; h < n; h++) {
				for (int w = 0; w < n; w++) {
					input(b, 8, h, w) = (move_num + 1) % 2;
				}
			}

			for (int i = move_num - 1, plane = 2; i >= 0 && i + 6 >= move_num; i -= 2, plane += 2) {
				int h = states[b][i] / n, w = states[b][i] % n;
				for (int p = plane; p < 8; p += 2) {
					input(b, p, h, w) = 0.0;
				}
			}
			for (int i = move_num - 2, plane = 3; i >= 0 && i + 6 >= move_num; i -= 2, plane += 2) {
				int h = states[b][i] / n, w = states[b][i] % n;
				for (int p = plane; p < 8; p += 2) {
					input(b, p, h, w) = 0.0;
				}
			}
		}
		std::vector<tensorflow::Tensor> outputs;
		//status = session->Run({ {"state",state} }, { "model/dense_layer_1/LogSoftmax","model/flatten_layer_3/Tanh" }, {}, &outputs); data type is float
		status = session->Run({ {"state",state} }, { "probability","value" }, {}, &outputs);
		if (!status.ok()) {
			std::cout << status.ToString() << std::endl;
		}
		
		//const Eigen::TensorMap<Eigen::Tensor<double, 1, Eigen::RowMajor>, Eigen::Aligned>& prediction = outputs[0].flat<double>();
		auto prediction = outputs[0].flat<double>();
		for (int i = 0; i < bs; i++) {
			std::vector<double> prob(n * n, 0);
			for (int pos = 0; pos < n * n; pos++) {
				int h = pos / n, w = pos % n;
				prob[(14 - h) * n + w] = exp(prediction(pos + i * n * n));
			}
			double sum = 1;
			for (int j = 0; j < states[i].size(); j++) {//mask the invalid position
				sum -= prob[states[i][j]];
				prob[states[i][j]] = 0;
			}
			for (int pos = 0; pos < n * n; pos++) {//Normalization
				prob[pos] /= sum;
			}
			prob.push_back(outputs[1].flat<double>()(i));
			//the probability of position and value of current state saved in one vecctor
			//value = *(outputs[1].flat<float>().data());if the batch size is 1
			promises[i].set_value(std::move(prob));
		}
	}

	int n, batchsize;
	std::string model_path;
	bool running;

	std::mutex lock;
	std::condition_variable cv;
	std::queue<task_type> tasks;
	std::unique_ptr<std::thread> loop;

	tensorflow::Session* session;
	tensorflow::Status status;
};
