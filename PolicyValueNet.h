#include <tensorflow/core/platform/env.h>
#include <tensorflow/core/public/session.h>
#include "Board.h"
#define NOMINMAX

using std::string;
using std::vector;

using tensorflow::Tensor;

class PolicyValueNet
{
public:
	PolicyValueNet(int n = 15, string model_path ="C:/yu/frozen_model.pb") {
		this->model_path = model_path;//"C:/yu/alphago/ahphago/tfgo/output_model/pb_model/frozen_model.pb"
		this->n = n;//"‪C:/yu/alphago/ahphago/tfgo/mode.pb"
		status = tensorflow::NewSession(tensorflow::SessionOptions(), &session);
		status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), model_path, &graph_def);
		status = session->Create(graph_def);
		if (!status.ok()) {
			std::cout << status.ToString()<<std::endl;
		}
	}
	vector<double> getprob(const Board& board, double& value) {
		Tensor state(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1,9,15,15 }));
		auto input = state.tensor<float, 4>();
		int step = board.move_seq.size();
		for (int k = 0; k < 8; k++) {
			for (int i = 0; i < n; i++)
				for (int j = 0; j < n; j++)
					input(0, k, i, j) = 0.0;
			if (k % 2 == 1) {
				for (int i = step - 2; i >= 0; i -= 2) {
					int h = board.move_seq[i] / n, w = board.move_seq[i] % n;
					input(0, k, h, w) = 1.0;
				}
			}
			else {
				for (int i = step - 1; i >= 0; i -= 2) {
					int h = board.move_seq[i] / n, w = board.move_seq[i] % n;
					input(0, k, h, w) = 1.0;
				}
			}
		}

		int plane, i, k;
		for (i = step - 1, plane = 2; i >= 0 && i + 6 >= step; i -= 2, plane += 2) {
			int h = board.move_seq[i] / n, w = board.move_seq[i] % n;
			for (k = plane; k < 8; k += 2) {
				assert(input(0, k, h, w) == 1);
				input(0, k, h, w) = 0.0;
			}
		}
		//printf("start deleting oppo move\n");
		for (i = step - 2, plane = 3; i >= 0 && i + 6 >= step; i -= 2, plane += 2) {
			int h = board.move_seq[i] / n, w = board.move_seq[i] % n;
			for (k = plane; k < 8; k += 2) {
				assert(input(0, k, h, w) == 1);
				input(0, k, h, w) = 0.0;
			}
		}

		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++) {
				if (step % 2 == 0)  input(0, 8, i, j) = 1.0;
				else input(0, 8, i, j) = 0.0;
			}


		//for (int k = 0; k < 8; k += 2) {
		//	for (int i = 0; i < 15; i++) {
		//		for (int j = 0; j < 15; j++)
		//			printf("%.0f ", input(0, k, i, j));
		//		printf("\n");
		//	}
		//	printf("\n\n");
		//}
		//printf("----------------------------------------------------------------------------------------\n");
		//for (int k = 1; k < 8; k += 2) {
		//	for (int i = 0; i < 15; i++) {
		//		for (int j = 0; j < 15; j++)
		//			printf("%.0f ", input(0, k, i, j));
		//		printf("\n");
		//	}
		//	printf("\n\n");
		//}
		//for (int i = 0; i < 15; i++) {
		//	for (int j = 0; j < 15; j++)
		//		printf("%.0f ", input(0, 8, i, j));
		//	printf("\n");
		//}

		//model / dense_layer_1 / LogSoftmax:0 model / flatten_layer_3 / Tanh : 0
		vector<Tensor> outputs;
		{
			std::lock_guard<std::mutex> lock(this->lock);
			//status = session->Run({ {"state_1",state} }, {}, { "model_1/dense_layer_1/LogSoftmax:0", "model_1/flatten_layer_3/Tanh:0" }, &outputs);

			//{ "model/dense_layer_1/LogSoftmax:0", "model/flatten_layer_3/Tanh:0" }
			status = session->Run({ {"state",state} }, { "model/dense_layer_1/LogSoftmax","model/flatten_layer_3/Tanh" }, {}, &outputs);
			std::cout << status.ToString() << std::endl;
		}
		//status = session->Run({ {"state",state} }, { "probability","value" }, {}, &outputs);
		//status = session->Run({ {"state",state} }, { "model/dense_layer_1/LogSoftmax","model/flatten_layer_3/Tanh" }, {}, &outputs);
		Tensor* probability = &outputs[0];
		//const Eigen::TensorMap<Eigen::Tensor<double, 1, Eigen::RowMajor>, Eigen::Aligned>& prediction = probability->flat<double>();
		const Eigen::TensorMap<Eigen::Tensor<float, 1, Eigen::RowMajor>, Eigen::Aligned>& prediction = probability->flat<float>();
		vector<double> prob(n * n, 0);
		//vector<double> prob;
		double total = 1;
		for (int i = 0; i < prediction.size(); i++) {
			int h = i / n, w = i % n;
			prob[(14 - h) * n + w] = exp(prediction(i));
		}
		//printf("this is ok\n");
		for (int i = 0; i < board.move_seq.size(); i++) {
			total -= prob[board.move_seq[i]];
			prob[board.move_seq[i]] = 0;
		}
		for (int i = 0; i < prob.size(); i++)
			prob[i] /= total;
		value = *(outputs[1].flat<float>().data());
		//printf("value is %f", value);
		//if (board.move_seq.size() == 2 && board.move_seq[0] == 113 && (board.move_seq[1] == 27||board.move_seq[1] == 106) )  value = 0.998;
		//value = *(outputs[1].data());
		return prob;
	}
	~PolicyValueNet() {
		session->Close();
		//delete session;
		session = nullptr;
	}

private:
	std::mutex lock;
	int n;
	string model_path;
	tensorflow::Session* session;
	tensorflow::Status status;
	tensorflow::GraphDef graph_def;
};