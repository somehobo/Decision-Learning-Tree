#include <iostream>
#include <vector>
#include <limits.h>
#include <cmath>
#include <map>
#include <mpi.h>
#include <string>
#include <set>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>

using namespace std;

struct split
{
  int b_index;
  bool leaf = false;
  float b_value;
  float b_gini;
  split *right = NULL;
  split *left = NULL;
  // left group at b_groups[0], right at b_groups[1]
  vector<vector<vector<float>>> b_groups;
};

struct get_single_split_params
{
  int threadIdx;
  int start;
  int end;
  struct split sp;
  vector<vector<float>> dataset;
};

vector<float> decision_tree(vector<vector<float>> train, vector<vector<float>> test,
                            int max_depth, int min_size);
vector<vector<float>> convert();
vector<vector<vector<float>>> c_v_split(vector<vector<float>> dataset, int n_folds);
float accuracy_metric(vector<float> actual, vector<float> predicted);
vector<float> evaluate_algorithm(vector<vector<float>> dataset, int n_folds, int max_depth, int min_size);
float gini_index(vector<vector<vector<float>>> groups, set<float> classes);
vector<vector<vector<float>>> test_split(int index, float value, vector<vector<float>> dataset);
split *to_terminal(vector<vector<float>> group);
void splitNode(split *node, int max_depth, int min_size, int depth);
split *get_split(vector<vector<float>> dataset);
void *get_single_split(void *threadp);
struct split *build_tree(vector<vector<float>> train, int max_depth, int min_size);
void print_tree(split *root, int depth = 0);
struct split *predict(split *node, vector<float> row);
vector<float> decision_tree(vector<vector<float>> train, vector<vector<float>> test,
                            int max_depth, int min_size);

#define NUM_CORES (2)
const int MAX_STRING = 256;
static const string fileName = "banknotes.txt";
static const string DEBUGFLAG = "-d";
bool debug = false;
int num_threads;

int main(int argc, char const *argv[])
{
  char hostname[MAX_STRING];
  char nodename[MPI_MAX_PROCESSOR_NAME];
  int comm_sz;
  int working_sz;
  int my_rank, namelen;

  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if (my_rank == 0)
  {
    int min_size;
    int max_depth;
    int n_folds;
    struct timespec startTime, stopTime;
    double accum;

    for (int i = 0; i < argc; i++)
    {
      if (argv[i] == DEBUGFLAG)
      {
        debug = true;
        cout << "Running in debug mode" << endl;
      }
    }

    cout << "Enter number of folds (the higher, the longer it takes)" << endl;
    cin >> n_folds;
    cout << "Enter max depth of the tree" << endl;
    cin >> max_depth;
    cout << "Enter minimum split data size" << endl;
    cin >> min_size;
    cout << "Working... this might take awhile, go get a coffee" << endl;

    if (clock_gettime(CLOCK_REALTIME, &startTime) == -1)
    {
      perror("clock gettime");
      exit(1);
    }

    vector<vector<float>> dataset = convert();
    vector<float> scores = evaluate_algorithm(dataset, n_folds, max_depth, min_size);
    cout << "Percentage accuracy per fold: " << endl;
    float average = 0;
    for (int i = 0; i < scores.size(); i++)
    {
      average += scores[i];
      cout << scores[i] << " ";
    }
    average = average / n_folds;
    cout << "Average accuracy = " << average << endl;

    if (clock_gettime(CLOCK_REALTIME, &stopTime) == -1)
    {
      perror("clock gettime");
      exit(1);
    }

    accum = (stopTime.tv_sec - startTime.tv_sec) +
            (double)(stopTime.tv_nsec - startTime.tv_nsec) /
                (double)1000000000;

    printf("Time elapsed %lf \n", accum);
  }
  else
  {
  }

  return 0;
}

vector<vector<float>> convert()
{ // convert text data to a 2d dataset
  if (debug)
  {
    cout << "convert" << endl;
  }

  ifstream bankNotes(fileName);
  string curNum;
  vector<vector<float>> dataset;
  vector<float> temp;
  int counter = 1; // for catching last number in a row
  if (bankNotes.is_open())
  {
    while (getline(bankNotes, curNum, ','))
    {
      temp.push_back(stof(curNum));
      counter++;
      if (counter % 4 == 0)
      { // to catch the last column of data seperateley
        getline(bankNotes, curNum);
        temp.push_back(stof(curNum));
        dataset.push_back(temp);
        temp.clear();
      }
    }
  }
  else
  {
    cout << "data file failed to open" << endl;
    exit(1);
  }
  return dataset;
}

// returns n different test sets, super cool
vector<vector<vector<float>>> c_v_split(vector<vector<float>> dataset, int n_folds)
{
  if (debug)
  {
    cout << "c_v_split" << endl;
    cout << "dataset size = " << dataset.size() << endl;
  }

  vector<vector<vector<float>>> dataset_split;
  vector<vector<float>> dataset_copy = dataset;
  int fold_size = dataset.size() / n_folds;
  for (int i = 0; i < n_folds; i++)
  {
    vector<vector<float>> fold;
    while (fold.size() < fold_size)
    {
      int index = rand() % (dataset_copy.size()); // random rows for testing
      fold.push_back(dataset_copy[index]);
      dataset_copy.erase(dataset_copy.begin() + index); // delete test row from set
    }
    // cout << "dataset_split.push_back(fold)" << endl;
    dataset_split.push_back(fold);
  }
  if (debug)
  {
    cout << "end c_v_split" << endl;
  }
  return dataset_split;
}

// percentage correct from the folds prediction
float accuracy_metric(vector<float> actual, vector<float> predicted)
{
  if (debug)
  {
    cout << "accuracy metric" << endl;
  }

  float correct = 0;
  for (int i = 0; i < actual.size(); i++)
  {
    if (actual[i] == predicted[i])
    {
      correct++;
    }
  }
  correct = (correct / float(actual.size())) * 100.0;
  return correct;
}

vector<float> evaluate_algorithm(vector<vector<float>> dataset, int n_folds, int max_depth, int min_size)
{
  if (debug)
  {
    cout << "evaluate algorithm" << endl;
  }
  vector<vector<vector<float>>> folds = c_v_split(dataset, n_folds);
  vector<float> scores; // keeps track of percentage correctness
  for (int i = 0; i < folds.size(); i++)
  { // for fold in folds
    // create dataset discluding current fold
    vector<vector<float>> train_set;
    vector<vector<float>> test_set;
    for (int j = 0; j < folds.size(); j++)
    {
      if (i != j)
      {
        // concactinate fold into train_set
        train_set.insert(train_set.end(), folds[j].begin(), folds[j].end());
      }
    }
    // copy the fold for the test and get rid of last line for a true
    // prediction
    for (int w = 0; w < folds[i].size(); w++)
    {
      vector<float> row_copy = folds[i][w];
      row_copy[row_copy.size() - 1] = -1;
      test_set.push_back(row_copy);
    }
    vector<float> predicted = decision_tree(train_set, test_set, max_depth, min_size);
    vector<float> actual;
    for (int n = 0; n < folds[i].size(); n++)
    {
      actual.push_back(folds[i][n][folds[i][n].size() - 1]); // grab last of each row
    }
    float accuracy = accuracy_metric(actual, predicted);
    scores.push_back(accuracy);
  }
  return scores;
}

// returns a score of how well a dataset is split
float gini_index(vector<vector<vector<float>>> groups, set<float> classes)
{
  if (debug)
  {
    // cout << "gini_index" << endl;
  }
  // count all samples at the current split point
  float n_instances = 0;
  for (int i = 0; i < groups.size(); i++)
  {
    n_instances += (float)groups[i].size();
  }
  // sum of all gini scores for each group
  float gini = 0.0;
  for (int i = 0; i < groups.size(); i++)
  { // outer g
    float size = (float)groups[i].size();
    if (size == 0)
    {
      continue;
    }
    float score = 0.0;
    for (int c = 0; c < classes.size(); c++)
    {
      float p = 0.0;
      for (int j = 0; j < groups[i].size(); j++)
      {
        if (groups[i][j][groups[i][j].size() - 1] == *classes.begin() + c)
        {
          p++;
        }
      }
      p = p / size;
      p = p * p;
      score = score + p;
    }
    gini += (1.0 - score) * (size / n_instances);
  }
  return gini;
}

// split the dataset with a given value
// returns 3 by x array with left and right contained
// results[0] = left, results[1] = right
vector<vector<vector<float>>> test_split(int index, float value, vector<vector<float>> dataset)
{
  vector<vector<float>> left;
  vector<vector<float>> right;
  for (int i = 0; i < dataset.size(); i++)
  { // i is row in dataset
    if (dataset[i][index] < value)
    {
      left.push_back(dataset[i]);
    }
    else
    {
      right.push_back(dataset[i]);
    }
  }
  vector<vector<vector<float>>> results;
  results.push_back(left);
  results.push_back(right);
  return results;
}

split *get_split(vector<vector<float>> dataset)
{
  if (debug)
  {
    cout << "get split" << endl;
  }
  pthread_t threads[NUM_CORES];
  get_single_split_params threadParams[NUM_CORES];
  vector<vector<vector<float>>> b_groups;
  split *b_split = new split;
  b_split->b_gini = 999;
  int sliceSize = dataset.size() / NUM_CORES;
  for (int i = 0; i < NUM_CORES; i++)
  {
    threadParams[i].start = i * sliceSize;
    threadParams[i].end = threadParams[i].start + sliceSize;
    threadParams[i].dataset = dataset;
    pthread_create(&threads[i], NULL, get_single_split, (void *)&(threadParams[i]));
  }

  for (int i = 0; i < NUM_CORES; i++)
  {
    pthread_join(threads[i], NULL);
    if (threadParams[i].sp.b_gini < b_split->b_gini)
    {
      b_split->b_gini = threadParams[i].sp.b_gini;
      b_split->b_groups = threadParams[i].sp.b_groups;
      b_split->b_index = threadParams[i].sp.b_index;
      b_split->b_value = threadParams[i].sp.b_value;
    }
  }
  if (debug)
  {
    cout << "in get split " << b_split->b_groups.size() << endl;
    cout << "get split end" << endl;
  }
  return b_split;
}

void *get_single_split(void *threadp)
{
  struct get_single_split_params *params = (get_single_split_params *)threadp;

  if (debug)
  {
    cout << "get_single_split, start = " << params->start << " end = " << params->end << endl;
  }

  set<float> class_values;
  // fill out set
  for (int i = 0; i < params->dataset.size(); i++)
  {
    class_values.insert(params->dataset[i][params->dataset[i].size() - 1]);
  }

  // find the best in the given section of rows
  params->sp = split();
  int b_index = 999;
  float b_value = 999;
  float b_gini = 999;
  vector<vector<vector<float>>> b_groups;
  for (int i = 0; i < params->dataset[0].size() - 1; i++)
  { // iterate thru all except gini num
    // each column
    for (int j = params->start; j < params->end; j++)
    { // for each row
      // pick a split value and test
      vector<vector<vector<float>>> groups = test_split(i, params->dataset[j][i], params->dataset);
      // check out dat mf gini score yo
      float gini = gini_index(groups, class_values);
      // check to see if the score is better
      if (gini < b_gini)
      {
        b_index = i;
        b_value = params->dataset[j][i];
        b_gini = gini;
        b_groups = groups;
      }
    }
  }
  params->sp.b_groups = b_groups;
  params->sp.b_index = b_index;
  params->sp.b_value = b_value;
  params->sp.b_gini = b_gini;
  return NULL;
}

split *to_terminal(vector<vector<float>> group)
{
  // cout<<"to terminal"<<endl;
  // returns vector of vector vectors with the result at 0,0,0
  vector<vector<vector<float>>> result;
  vector<float> outcomes;
  set<float> setOutcomes;
  for (int i = 0; i < group.size(); i++)
  {
    // get all the class values
    outcomes.push_back(group[i][group[i].size() - 1]);
    setOutcomes.insert(group[i][group[i].size() - 1]);
  }
  // return the value that shows up most
  float max = -999;
  int maxCount = 0;
  for (int i = 0; i < setOutcomes.size(); i++)
  {
    int curCount = 0;
    for (int j = 0; j < outcomes.size(); j++)
    {
      if (*setOutcomes.begin() + i == outcomes[j])
      {
        curCount++;
      }
    }
    if (curCount > maxCount)
    {
      max = *setOutcomes.begin() + i;
      maxCount = curCount;
    }
  }
  // create the leaf node split
  vector<float> inside;
  inside.push_back(max);
  vector<vector<float>> middle;
  middle.push_back(inside);
  result.push_back(middle);
  split *terminal = new split;
  terminal->b_groups = result;
  terminal->leaf = true;
  return terminal;
}

// create child nodes from a split or terminal(end branch)
void splitNode(split *node, int max_depth, int min_size, int depth)
{
  if (debug)
  {
    cout << "split node" << endl;
    cout << "b_groups gini = " << node->b_gini << endl;
    cout << node->b_groups.size() << endl;
  }
  vector<vector<float>> left = node->b_groups[0];
  vector<vector<float>> right = node->b_groups[1];
  // delete old data
  // check for no split case(no data in children)
  if (left.size() == 0 || right.size() == 0)
  {
    // combine arrays in left
    for (int i = 0; i < right.size(); i++)
    {
      left.push_back(right[i]);
    }
    node->left = to_terminal(left);
    node->right = to_terminal(left);
    return;
  }
  // check for max depth
  // if true end the branch with to leafs
  if (depth >= max_depth)
  {
    node->left = to_terminal(left);
    node->right = to_terminal(right);
    return;
  }
  // process the left child
  if (left.size() <= min_size)
  {
    node->left = to_terminal(left);
  }
  else
  {
    node->left = get_split(left);
    splitNode(node->left, max_depth, min_size, depth + 1);
  }

  // process the right child
  if (right.size() <= min_size)
  {
    node->right = to_terminal(right);
  }
  else
  {
    node->right = get_split(right);
    splitNode(node->right, max_depth, min_size, depth + 1);
  }
}

struct split *build_tree(vector<vector<float>> train, int max_depth, int min_size)
{
  if (debug)
  {
    cout << "build_tree (max_depth=" << max_depth << ", min_size=" << min_size << ")" << endl;
  }
  split *root = get_split(train);
  splitNode(root, max_depth, min_size, 1);
  return root;
}

void print_tree(split *root, int depth)
{
  // quick depth first print
  if (root == NULL)
  {
    return;
  }
  for (int i = 0; i <= depth; i++)
  {
    cout << " ";
  }
  if (root->leaf)
  {
    cout << root->b_groups[0][0][0] << endl;
  }
  else
  {
    cout << root->b_value << endl;
  }
  print_tree(root->left, depth + 1);
  print_tree(root->right, depth + 1);
}

struct split *predict(split *node, vector<float> row)
{
  if (row[node->b_index] < node->b_value)
  {
    if (!node->left->leaf)
    {
      return predict(node->left, row);
    }
    else
    {
      return node->left;
    }
  }
  else
  {
    if (!node->right->leaf)
    {
      return predict(node->right, row);
    }
    else
    {
      return node->right;
    }
  }
}

vector<float> decision_tree(vector<vector<float>> train, vector<vector<float>> test,
                            int max_depth, int min_size)
{
  split *root = build_tree(train, max_depth, min_size);
  vector<float> predictions;
  for (int i = 0; i < test.size(); i++)
  {
    split *prediction = predict(root, test[i]);
    predictions.push_back(prediction->b_groups[0][0][0]);
  }
  return predictions;
}
