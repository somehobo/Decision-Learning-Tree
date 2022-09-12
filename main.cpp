#include <iostream>
#include <vector>
#include <limits.h>
#include <cmath>
#include <map>
#include <string>
#include <set>
#include <fstream>      /* read files */
#include <stdlib.h>     /* atoi */

using namespace std;

static const string fileName = "banknotes.cpp";

vector<float> decision_tree(vector<vector<float> > train, vector<vector<float> > test,
                  int max_depth, int min_size);
//struct to store split values
struct split{
  int b_index;
  bool leaf = false;
  float b_value;
  split* right = NULL;
  split* left = NULL;
  vector<vector<vector<float> > > b_groups;
};

vector<vector<float> > convert(){//convert text data to a 2d dataset
  ifstream bankNotes (fileName);
  string curNum;
  vector<vector<float> > dataset;
  vector<float> temp;
  int counter = 1;//for catching last number in a row
  if (bankNotes.is_open()){
    while(getline(bankNotes, curNum,',')){
      temp.push_back(stof(curNum));
      counter++;
      if(counter % 4 == 0){//to catch the last column of data seperateley
        getline(bankNotes, curNum);
        temp.push_back(stof(curNum));
        dataset.push_back(temp);
        temp.clear();
      }
    }
  }
  return dataset;
}

//returns n different test sets, super cool
vector<vector<vector<float> > >c_v_split(vector<vector<float> > dataset
                                        , int n_folds){


  vector<vector<vector<float> > > dataset_split;
  vector<vector<float> > dataset_copy = dataset;
  int fold_size = dataset.size()/n_folds;
  for(int i = 0; i < n_folds; i++){
    vector<vector<float> > fold;
    while(fold.size() < fold_size){
      int index = rand() % (dataset_copy.size()); //random rows for testing
      fold.push_back(dataset_copy[index]);
      dataset_copy.erase(dataset_copy.begin()+index-1);//delete test row from set
    }
    dataset_split.push_back(fold);
  }
  return dataset_split;
}

//percentage correct from the folds prediction
float accuracy_metric(vector<float> actual, vector<float> predicted){
  float correct = 0;
  for (int i = 0; i < actual.size(); i++) {
    if(actual[i] == predicted[i]){
      correct++;
    }
  }
  correct = (correct/float(actual.size()))*100.0;
  return correct;
}

vector<float> evaluate_algorithm(vector<vector<float> > dataset, int n_folds, int max_depth
                        , int min_size){
  vector<vector<vector<float> > > folds = c_v_split(dataset, n_folds);
  vector<float> scores; //keeps track of percentage correctness
  for (int i = 0; i < folds.size(); i++) {//for fold in folds
    //create dataset discluding current fold
    vector<vector<float> > train_set;
    vector<vector<float> > test_set;
    for(int j = 0; j < folds.size(); j++){
      if(i != j){
        //concactinate fold into train_set
        train_set.insert(train_set.end(), folds[j].begin(), folds[j].end());
      }
    }
    //copy the fold for the test and get rid of last line for a true
    //prediction
    for(int w = 0; w < folds[i].size(); w++){
      vector<float> row_copy = folds[i][w];
      row_copy[row_copy.size()-1] = -1;
      test_set.push_back(row_copy);
    }
    vector<float> predicted = decision_tree(train_set,test_set,max_depth,min_size);
    vector<float> actual;
    for(int n = 0; n < folds[i].size(); n++){
      actual.push_back(folds[i][n][folds[i][n].size()-1]);//grab last of each row
    }
    float accuracy = accuracy_metric(actual, predicted);
    scores.push_back(accuracy);
  }
  return scores;
}

//returns a score of how well a dataset is split
float gini_index(vector<vector<vector<float> > > groups, set<float> classes){
  //cout<<"gini"<<endl;
  //count all samples at the current split point
  float n_instances = 0;
  for(int i = 0; i < groups.size(); i++){
    n_instances += (float)groups[i].size();
  }
  //sum of all gini scores for each group
  float gini = 0.0;
  for(int i = 0; i < groups.size(); i++){//outer g
    float size = (float)groups[i].size();
    if(size == 0){
      continue;
    }
    float score = 0.0;
    for(int c = 0; c < classes.size(); c++){
      float p = 0.0;
      for(int j = 0; j < groups[i].size(); j++){
        if(groups[i][j][groups[i][j].size()-1] == *classes.begin()+c){
          p++;
        }
      }
      p = p/size;
      p = p*p;
      score = score + p;
    }
    gini += (1.0 - score) * (size / n_instances);
  }
  return gini;
}

//split the dataset with a given value
//returns 3 by x array with left and right contained
//results[0] = left, results[1] = right
vector<vector<vector<float> > > test_split(int index, float value, vector<vector<float> > dataset){
  //cout<<"test split"<<endl;
  vector<vector<float> > left;
  vector<vector<float> > right;
  for(int i = 0; i < dataset.size(); i++){//i is row in dataset
    if(dataset[i][index] < value){
      left.push_back(dataset[i]);
    } else{
      right.push_back(dataset[i]);
    }
  }
  vector<vector<vector<float> > > results;
  results.push_back(left);
  results.push_back(right);
  return results;
}

split* get_split(vector<vector<float> > dataset){
  //cout<<"get split"<<endl;
  set<float> class_values;
  //fill out set
  for (int i = 0; i < dataset.size(); i++) {
    class_values.insert(dataset[i][dataset[i].size()-1]);
  }

  //b = best hehe
  int b_index = 999;
  float b_value = 999;
  float b_score = 999;
  vector<vector<vector<float> > > b_groups;
  //greedy alg to find best split
  for(int i = 0; i < dataset[0].size()-1; i++){//iterate thru all except gini num
    //each column
    for(int j = 0; j < dataset.size(); j++){//for each row
      //pick a split value and test
      vector<vector<vector<float> > > groups = test_split(i, dataset[j][i], dataset);
      //check out dat mf gini score yo
      float gini = gini_index(groups, class_values);
      //check to see if the score is better
      if(gini < b_score){
        b_index = i;
        b_value = dataset[j][i];
        b_score = gini;
        b_groups = groups;
      }
    }
  }
  //assemble best split object
  split* b_split = new split;
  b_split->b_groups = b_groups;
  b_split->b_index = b_index;
  b_split->b_value = b_value;
  return b_split;
}

split* to_terminal(vector<vector<float> > group){
  //cout<<"to terminal"<<endl;
  //returns vector of vector vectors with the result at 0,0,0
  vector<vector<vector<float> > > result;
  vector<float> outcomes;
  set<float> setOutcomes;
  for(int i = 0; i < group.size(); i++){
    //get all the class values
    outcomes.push_back(group[i][group[i].size()-1]);
    setOutcomes.insert(group[i][group[i].size()-1]);
  }
  //return the value that shows up most
  float max = -999;
  int maxCount = 0;
  for(int i = 0; i < setOutcomes.size(); i++){
    int curCount = 0;
    for(int j = 0; j < outcomes.size(); j++){
      if(*setOutcomes.begin()+i == outcomes[j]){
        curCount++;
      }
    }
    if(curCount > maxCount){
      max = *setOutcomes.begin()+i;
      maxCount = curCount;
    }
  }
  //create the leaf node split
  vector<float> inside;
  inside.push_back(max);
  vector<vector<float> > middle;
  middle.push_back(inside);
  result.push_back(middle);
  split* terminal = new split;
  terminal->b_groups = result;
  terminal->leaf = true;
  return terminal;
}

//create child nodes from a split or terminal(end branch)
void splitNode(split* node, int max_depth, int min_size, int depth){
  vector<vector<float> > left = node->b_groups[0];
  vector<vector<float> > right = node->b_groups[1];
  //delete old data
  //check for no split case(no data in children)
  if(left.size() == 0 || right.size() == 0){
    //combine arrays in left
    for (int i = 0; i < right.size(); i++) {
      left.push_back(right[i]);
    }
    node->left = to_terminal(left);
    node->right = to_terminal(left);
    return;
  }
  //check for max depth
  //if true end the branch with to leafs
  if (depth >= max_depth){
    node->left = to_terminal(left);
    node->right = to_terminal(right);
    return;
  }
  //process the left child
  if(left.size() <= min_size){
    node->left = to_terminal(left);
  } else{
    node->left = get_split(left);
    splitNode(node->left, max_depth, min_size, depth+1);
  }

  //process the right child
  if(right.size() <= min_size){
    node->right = to_terminal(right);
  } else{
    node->right = get_split(right);
    splitNode(node->right, max_depth, min_size, depth+1);
  }
}

struct split* build_tree(vector<vector<float> > train, int max_depth, int min_size){
  struct split* root = get_split(train);
  splitNode(root, max_depth, min_size, 1);
  return root;
}

void print_tree(split* root, int depth = 0){
  //quick depth first print
  if(root == NULL){
    return;
  }
  for(int i = 0; i <= depth; i++){
    cout<<" ";
  }
  if(root->leaf){
    cout<<root->b_groups[0][0][0]<<endl;
  } else {
    cout<<root->b_value<<endl;
  }
  print_tree(root->left, depth + 1);
  print_tree(root->right, depth + 1);
}
struct split* predict(split* node, vector<float> row){
  if(row[node->b_index] < node->b_value){
    if(!node->left->leaf){
      return predict(node->left, row);
    } else {
      return node->left;
    }
  } else {
    if(!node->right->leaf){
      return predict(node->right, row);
    } else {
      return node->right;
    }
  }
}

vector<float> decision_tree(vector<vector<float> > train, vector<vector<float> > test,
                  int max_depth, int min_size){
  split* root = build_tree(train, max_depth, min_size);
  vector<float> predictions;
  for (int i = 0; i < test.size(); i++) {
    split* prediction = predict(root, test[i]);
    predictions.push_back(prediction->b_groups[0][0][0]);
  }
  return predictions;
}

int main(int argc, char const *argv[]) {
  //Option 1 is the exaluate algorthm and it takes AWHILE to run due to the
  //size of the test case.
  //within option 1, you can decide how many folds to evaluate

  //Option 2 is the single run where it outputs a tree where the amount of
  //spaces indicates the node's depth.

  //within both options you can decide the depth of the tree and the smallest
  //grouping of data you want to split.

  //you can either follow the inputs i give below or mess around with it
  //to see the different accuracys which is pretty fun.

  //input in the order you see
  //option 1 tests
  //highest avg accuracy to lowest due to tree depth
  //1,3,5,30

  //1,4,4,50

  //1,2,2,100

  //option 2
  //2,5,30

  //2,4,50

  //2,1,100



  int decider;
  cout<<"Evaluate? or single run? Enter a 1 or a 2 "<<endl;
  cin>>decider;
  if(decider == 1){
    cout<<"Enter number of folds (the higher, the longer it takes)"<<endl;
    int n_folds;
    cin>>n_folds;
    cout<<"Enter max depth of the tree"<<endl;
    int max_depth;
    cin>>max_depth;
    cout<<"Enter minimum split data size"<<endl;
    int min_size;
    cin>>min_size;
    cout<<"Working... this might take awhile, go get a coffee"<<endl;
    vector<vector<float> >dataset = convert();
    vector<float> scores = evaluate_algorithm(dataset, n_folds, max_depth, min_size);
    cout<<"Percentage accuracy per fold: "<<endl;
    float average = 0;
    for (int i = 0; i < scores.size(); i++) {
      average += scores[i];
      cout<<scores[i]<<" ";
    }
    average = average/n_folds;
    cout<<"Average accuracy = "<<average<<endl;
  } else if(decider == 2){
    cout<<"Enter max depth of the tree"<<endl;
    int max_depth;
    cin>>max_depth;
    cout<<"Enter minimum split data size"<<endl;
    int min_size;
    cin>>min_size;
    cout<<"Working..."<<endl;
    vector<vector<float> > dataset = convert();
    split* root = build_tree(dataset, max_depth, min_size);
    print_tree(root);
  }

  return 0;
}
