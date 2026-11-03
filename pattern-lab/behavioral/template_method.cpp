#include <iostream>
#include <string>
using namespace std;

class ModelTrainer {
public:
    virtual ~ModelTrainer() = default;

    void trainPipeline(const string &dataPath) const {
        loadData(dataPath);
        preprocessData();
        trainModel();
        evaluateModel();
        saveModel();
    }

protected:
    static void loadData(const string &path) {
        cout << "[Common] Loading dataset from " << path << "\n";
    }

    virtual void preprocessData() const {
        cout << "[Common] Splitting into train/test and normalizing\n";
    }

    virtual void trainModel() const = 0;
    virtual void evaluateModel() const = 0;

    virtual void saveModel() const {
        cout << "[Common] Saving model to disk as default format\n";
    }
};

class NeuralNetworkTrainer : public ModelTrainer {
protected:
    void trainModel() const override {
        cout << "[NeuralNet] Training Neural Network for 100 epochs\n";
    }

    void evaluateModel() const override {
        cout << "[NeuralNet] Evaluating accuracy and loss on validation set\n";
    }

    void saveModel() const override {
        cout << "[NeuralNet] Serializing network weights to .h5 file\n";
    }
};

class DecisionTreeTrainer : public ModelTrainer {
protected:
    void trainModel() const override {
        cout << "[DecisionTree] Building decision tree with max_depth=5\n";
    }

    void evaluateModel() const override {
        cout << "[DecisionTree] Computing classification report (precision/recall)\n";
    }
};

int main() {
    cout << "=== Neural Network Training ===\n";
    NeuralNetworkTrainer nnTrainer;
    nnTrainer.trainPipeline("data/images/");

    cout << "\n=== Decision Tree Training ===\n";
    DecisionTreeTrainer dtTrainer;
    dtTrainer.trainPipeline("data/iris.csv");

    return 0;
}
