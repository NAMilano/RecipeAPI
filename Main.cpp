#include "HttpClient.h"
#include <conio.h> // to use getch 
#include <shlwapi.h>	// To launch the web browser
#include <fstream> 
#include <crtdbg.h> 

#pragma warning( push )
#pragma warning(disable:26819)
#include "json.hpp"
#pragma warning( pop )
using json = nlohmann::json;
using namespace std;

class RecipeAPI : public HttpClient {
public:
	// Storage of all important information of a recipe
	struct Recipe {
		// Storage of all important nutritional values of the recipe
		struct Nutrition {
			double calories;
			double protein;
			double fat;
			double carbs;
			double cholesterol;
			double satFats;
			double fiber;
			double sodium;
			double sugar;
		};
		int id{};
		string title;
		string summary;
		int likes{};
		int readyTime{};
		string sourceUrl;
		string image;
		vector<string> cuisines;
		vector<string> dishType;
		vector<string> diets;
		Nutrition nutrition{};
	};
	// Destructor
	~RecipeAPI() {
		// Loops through entire array deleting the pointers
		for (int i = 0; i < maxRecipes; i++) {
			delete recipes[i];
		}
		// Deletes the pointer array
		delete[] recipes;
	};
	// Constructor
	RecipeAPI() {
		// Current max recipes for each API call
		maxRecipes = 10;
		// creates an array of pointers of Recipe
		recipes = new Recipe * [maxRecipes];
		// sets all pointers to null
		for (int i = 0; i < maxRecipes; i++) {
			recipes[i] = nullptr;
		}
	};
	// Opens the given file and stores all of its data in jsonData
	void openFile(string fileName) {
		// current file path of the test data
		const std::string DATA_FILE_PATH = "TestData\\";
		// open file
		std::ifstream testFile(DATA_FILE_PATH + fileName, std::ios::binary);
		// check to see if file opened
		if (!testFile.is_open())
			throw std::exception("Cannot open test file!");

		// Read the file into a vector of chars so that it is in the same format at what "HttpServer.h" will give us.
		std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)), std::istreambuf_iterator<char>());
		jsonData = fileContents;
	}
	// wrapper function to allow user to call the main parse from main - used for testdata files
	void parse() {
		const json& jf = json::parse(jsonData);
		// call main parsing function 
		parse(jf);
	}
	// formats and displays all recipe information - used with overloaded operator <<
	void displayRecipes(ostream& cout) {
		fixSummary();
		// loops through all recipes displaying their information
		for (int i = 0; i < recipeCount; i++) {
			Recipe current = *recipes[i];
			cout << i + 1 << ")\tID: " << current.id << "\t\tTitle: " << current.title << "\t\tCook time: " << current.readyTime << " minutes\t\tLikes: " << current.likes << endl;
			cout << "\tAll Cuisines: " << getAllCuisines(current) << "\n\tAll Diets: " << getAllDiets(current) << "\n\tAll Dish Types: " << getAllDishTypes(current) << endl;
			cout << "\t----------------------------  Nutrition  ----------------------------" << getAllNutrition(current) << endl;
			cout << "\t---------------------------------------------------------------------" << endl;
			cout << "\tSource website: " << current.sourceUrl << endl;
			cout << "\tSummary: " << current.summary << endl << endl;
		}
	}
	void resultsToFile() {
		// current file path for the results
		const std::string DATA_FILE_PATH = "TestData\\";
		ofstream results;
		results.open(DATA_FILE_PATH + "results.txt");

		results << "Total results: " + to_string(totalResults) << endl;
		results << "Results beign shown: " + to_string(recipeCount) << endl << endl << endl;
		displayRecipes(results);

		results.close();
	}
	// displays total results matching the user query parameters
	void displayStats() {
		cout << "\n----------------------------------------------------------------------" << endl;
		cout << "----                                                              ----" << endl;
		cout << "----------------------------------------------------------------------" << endl;
		cout << "----      ---- Total results matching query: " << totalResults << endl;
		cout << "----------------------------------------------------------------------" << endl;
		cout << "----                                                              ----" << endl;
		cout << "----------------------------------------------------------------------" << endl << endl;
	}
	// launches user web browser to display the desired recipe
	void displayBrowser() {
		int index;
		string viewMore;
		// gets the index number the user wishes to view
		cout << "\nEnter the desired index number: ";
		cin >> index;
		// check if the index requested is a valid index value
		if (index > 0 && index <= recipeCount) {
			// launch web browser with desired recipe - subtract one to match arrays starting from 0
			ShellExecuteA(NULL, "open", recipes[index - 1]->sourceUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
			// check if user wants to view more recipes from the current page
			cout << "\nDo you wish to view another recipe (yes or no): ";
			cin >> viewMore;
			viewMore = convertLower(viewMore);
			// if user inputs yes or y use a recursive function 
			if (viewMore == "yes" || viewMore == "y")
				displayBrowser();
		}
		// if user inputs an invalid index number call a recursive function until they input a valid index
		else {
			cout << "Not a valid index number. Please try again." << endl;
			displayBrowser();
		}
	}
	// displays continue message and gets user input about UP/DOWN to paginate, V  to view recipe, or quit program
	int continueMessage() {
		int c1;
		int c2;
		cout << "Use the UP and DOWN arrow keys to move through the pages. Use 'v' to view a recipe. Any other input to exit the program.";
		c1 = _getch();
		if (c1 == 224) {
			c2 = _getch();
			// if user inputs down arrow while already on offset 0 display message and call recursive function for them to choose again
			if (c2 == 80 && offset == 0) {
				cout << "\nAlready on the first page." << endl;
				c1 = continueMessage();
			}
			// if user inputs up arrow while already on the max offset display message and call recursive function for them to choose again
			if (c2 == 72 && offset >= totalResults) {
				cout << "\nAlready on the last" << endl;
				c1 = continueMessage();
			}
		}
		if (c1 == 224) {
			return c2;
		}
		else {
			return c1;
		}
	}
	// changes offset to display the pagination and prepare the program variables/containers for another API call
	string paginate(int input, string offset) {
		const int ARROW_UP = 72;
		const int ARROW_DOWN = 80;
		const int V_KEY = 118;
		int temp;
		// if input is up arrow add 10 to offset
		if (input == ARROW_UP) {
			temp = stoi(offset);
			temp += 10;
			offset = to_string(temp);
		}
		// if input is down arrow subtract 10 to offset
		if (input == ARROW_DOWN) {
			temp = stoi(offset);
			temp -= 10;
			offset = to_string(temp);
		}
		// call deletememory to prepare for next API call
		DeleteMemory();
		// return updated offset
		return offset;
	}
	// returns the recipes at the desired position
	const Recipe* getRecipe(const int i) {
		return recipes[i];
	}
	// concatenates all cuisines together seperated by comma
	string getAllCuisines(Recipe r) {
		string cuisines = "";
		size_t size = r.cuisines.size();
		// loops through all cuisines
		for (unsigned int x = 0; x < size; x++)
			cuisines += r.cuisines[x] + ", ";
		// if there is at least one cuisine call pop_back() twice to erase the trailing comma and space
		if (size > 0) {
			cuisines.pop_back();
			cuisines.pop_back();
		}
		return cuisines;
	}
	// concatenates all diets together seperated by comma
	string getAllDiets(Recipe r) {
		string diets = "";
		size_t size = r.diets.size();
		// loops through all diets
		for (unsigned int x = 0; x < size; x++)
			diets += r.diets[x] + ", ";
		// if there is at least one diet call pop_back() twice to erase the trailing comma and space
		if (size > 0) {
			diets.pop_back();
			diets.pop_back();
		}
		return diets;
	}
	// concatenates all dish types together seperated by comma
	string getAllDishTypes(Recipe r) {
		string types = "";
		size_t size = r.dishType.size();
		// loops through all dish types
		for (unsigned int x = 0; x < size; x++)
			types += r.dishType[x] + ", ";
		// if there is at least one dish type call pop_back() twice to erase the trailing comma and space
		if (size > 0) {
			types.pop_back();
			types.pop_back();
		}
		return types;
	}
	// concatenates all nutrional information together
	string getAllNutrition(Recipe r) {
		string nutrition = "";
		nutrition = "\n\tCalories: " + to_string(int(r.nutrition.calories)) + "\tProtein: " + to_string(int(r.nutrition.protein)) + "\tFat: " + to_string(int(r.nutrition.fat)) + "\tCarbs: " + to_string(int(r.nutrition.carbs)) + "\tCholesterol: " + to_string(int(r.nutrition.cholesterol))
			+ "\n\tSaturated Fats: " + to_string(int(r.nutrition.satFats)) + "\tFiber: " + to_string(int(r.nutrition.fiber)) + "\tSodium: " + to_string(int(r.nutrition.sodium)) + "\tSugar: " + to_string(int(r.nutrition.sugar));
		return nutrition;
	}
	// template function to call any of the Compare functions in the numerous CompareXXX strucst
	template<class T>
	void bubbleSort(bool ascending) {
		T xx{};
		// sorts all recipes in ascending direction
		if (ascending == true) {
			for (int iteration = 1; iteration < recipeCount; iteration++) {
				for (int index = 0; index < recipeCount - iteration; index++) {
					if (xx.Compare(recipes[index], recipes[index + 1])) {
						auto temp = recipes[index];
						recipes[index] = recipes[index + 1];
						recipes[index + 1] = temp;
					}
				}
			}
		}
		// sorts all recipes in descending direction 
		if (ascending == false) {
			for (int iteration = 1; iteration < recipeCount; iteration++) {
				for (int index = 0; index < recipeCount - iteration; index++) {
					if (xx.Compare(recipes[index], recipes[index + 1]) == false) {
						auto temp = recipes[index];
						recipes[index] = recipes[index + 1];
						recipes[index + 1] = temp;
					}
				}
			}
		}
	}
	// intro message
	void intro() {
		cout << "----------------------------------------------------------------------" << endl;
		cout << "----                                                              ----" << endl;
		cout << "----------------------------------------------------------------------" << endl;
		cout << "----                ---- Welcome to RecipeAPI ----                ----" << endl;
		cout << "----------------------------------------------------------------------" << endl;
		cout << "----         ---- Please fill out the questions below ----        ----" << endl;
		cout << "----            ---- to generate your custom query ----           ----" << endl;
		cout << "----------------------------------------------------------------------" << endl;
		cout << "----                                                              ----" << endl;
		cout << "----------------------------------------------------------------------" << endl << endl << endl;
	}
	// get all user requirments for their custom query - answers stored in userAnswers map 
	void getUserReq() {
		string input;
		// gets main query of title/name of recipe/food - this is a required field loops till user inputs something
		while (input == "") {
			cout << "Enter the name of the recipe/item: ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("query", input));
			if (input == "")
				cout << "This is a required field. Please input something." << endl;
		}
		// rules for following questions
		cout << "\nFor the rest of the questions you can skip to the next question by pressing enter" << endl;
		cout << "The questions marked with (M) allow for multiple answers being seperated by a comma (ex. italian, greek, indian)" << endl << endl;

		cout << "-----All of the valid cuisines-----" << "\nAfrican, American, British, Cajun, Caribbean, Chinese, Easter European, European, French" <<
			"\nGerman, Greek, Indian, Irish, Italian, Japanese, Jewish, Korean, Latin American" <<
			"\nMediterranean, Mexican, Middle Eastern, Nordic, Southern, Spanish, Thai, Vietnamese" << endl;
		cout << "Enter the desired cuisine (M): ";
		getline( cin, input, '\n');
		userAnswers.insert(pair<string, string>("cuisine", input));

		cout << "\n-----All of the valid diets-----" << "\nPescetarian, Lacto Vegetarian, Ovo Vegetarian, Vegan, Paleo, Primal, Vegetarian" << endl;
		cout << "Enter any diets the recipes must be compliant with (M): ";
		getline(cin, input, '\n');
		userAnswers.insert(pair<string, string>("diet", input));

		cout << "\n-----All of the valid intolerances-----" << "\nDairy, Egg, Gluten, Peanut, Sesame, Seafood, Shellfish, Soy, Sulfite, Tree Nut, Wheat" << endl;
		cout << "Enter any intolerances the recipes must avoid (M): ";
		getline(cin, input, '\n');
		userAnswers.insert(pair<string, string>("intolerances", input));

		cout << "\nEnter any ingredients that you want to be included in the recipes (M): ";
		getline(cin, input, '\n');
		userAnswers.insert(pair<string, string>("includeIngredients", input));

		cout << "\nEnter any ingredients that must not be included in the recipes (M): ";
		getline(cin, input, '\n');
		userAnswers.insert(pair<string, string>("excludeIngredients", input));

		cout << "\n-----All of the valid recipes types-----" << "\nMain Course, Side Dish, Dessert, Appetizer, Salad, Bread, Breakfast, Soup, Beverage, Sauce, Drink" << endl;
		cout << "Enter the type of dish you desired: ";
		getline(cin, input, '\n');
		userAnswers.insert(pair<string, string>("type", input));

		cout << "\nEnter the maximum amount of time to make the recipes (in minutes ex. 60): ";
		getline(cin, input, '\n');
		if (input == "") {
			input = "10000";
		}
		userAnswers.insert(pair<string, string>("maxReadyTime", input));
		// check if user wants to do a more specific search inlcuding nutritional information
		string continueQuestions;
		cout << "\nDo you wish to specify the min/max of nutritional values (yes/no): ";
		getline(cin, continueQuestions, '\n');
		
		// if user inputs yes ask all nutritional infomration questions 
		if (continueQuestions == "yes") {
			cout << "\nEach of the nutrients will have a marking indicating the unit of measurement.\n                -----(G) for grams - (MG) for milligrams-----                ";

			cout << "\nEnter the minimum amount of carbs in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minCarbs", input));
			cout << "\nEnter the maximum amount of carbs in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxCarbs", input));

			cout << "\nEnter the minimum amount of protein in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minProtein", input));
			cout << "\nEnter the maximum amount of protein in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxProtein", input));

			cout << "\nEnter the minimum amount of calories in the recipe: ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minCalories", input));
			cout << "\nEnter the maximum amount of calories in the recipe: ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxCalories", input));

			cout << "\nEnter the minimum amount of fat in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minFat", input));
			cout << "\nEnter the maximum amount of fat in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxFat", input));

			cout << "\nEnter the minimum amount of cholesterol in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minCholesterol", input));
			cout << "\nEnter the maximum amount of cholesterol in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxCholesterol", input));

			cout << "\nEnter the minimum amount of saturated fat in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minSaturatedFat", input));
			cout << "\nEnter the maximum amount of saturated fat in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxSaturatedFat", input));

			cout << "\nEnter the minimum amount of fiber in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minFiber", input));
			cout << "\nEnter the maximum amount of fiber in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxFiber", input));

			cout << "\nEnter the minimum amount of sodium in the recipe (MG): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minSodium", input));
			cout << "\nEnter the maximum amount of sodium in the recipe (MG): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxSodium", input));

			cout << "\nEnter the minimum amount of sugar in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("minSugar", input));
			cout << "\nEnter the maximum amount of sugar in the recipe (G): ";
			getline(cin, input, '\n');
			userAnswers.insert(pair<string, string>("maxSugar", input));

			// fix any answers the user chose to skip
			fixAnswers();
		}
		// fill all nutritional values with a default value
		else {
			userAnswers.insert(pair<string, string>("minCarbs", "0"));
			userAnswers.insert(pair<string, string>("maxCarbs", "10000"));
			userAnswers.insert(pair<string, string>("minProtein", "0"));
			userAnswers.insert(pair<string, string>("maxProtein", "10000"));
			userAnswers.insert(pair<string, string>("minCalories", "0"));
			userAnswers.insert(pair<string, string>("maxCalories", "10000"));
			userAnswers.insert(pair<string, string>("minFat", "0"));
			userAnswers.insert(pair<string, string>("maxFat", "10000"));
			userAnswers.insert(pair<string, string>("minCholesterol", "0"));
			userAnswers.insert(pair<string, string>("maxCholesterol", "10000"));
			userAnswers.insert(pair<string, string>("minSaturatedFat", "0"));
			userAnswers.insert(pair<string, string>("maxSaturatedFat", "10000"));
			userAnswers.insert(pair<string, string>("minFiber", "0"));
			userAnswers.insert(pair<string, string>("maxFiber", "10000"));
			userAnswers.insert(pair<string, string>("minSodium", "0"));
			userAnswers.insert(pair<string, string>("maxSodium", "10000"));
			userAnswers.insert(pair<string, string>("minSugar", "0"));
			userAnswers.insert(pair<string, string>("maxSugar", "10000"));
		}
	}
	// fills any answer the user skipped with a default value
	void fixAnswers() {
		if (userAnswers["minCarbs"] == "") {
			userAnswers["minCarbs"] = "0";
		}
		if (userAnswers["maxCarbs"] == "") {
			userAnswers["maxCarbs"] = "10000";
		}

		if (userAnswers["minProtein"] == "") {
			userAnswers["minProtein"] = "0";
		}
		if (userAnswers["maxProtein"] == "") {
			userAnswers["maxProtein"] = "10000";
		}

		if (userAnswers["minCalories"] == "") {
			userAnswers["minCalories"] = "0";
		}
		if (userAnswers["maxCalories"] == "") {
			userAnswers["maxCalories"] = "10000";
		}

		if (userAnswers["minFat"] == "") {
			userAnswers["minFat"] = "0";
		}
		if (userAnswers["maxFat"] == "") {
			userAnswers["maxFat"] = "10000";
		}

		if (userAnswers["minCholesterol"] == "") {
			userAnswers["minCholesterol"] = "0";
		}
		if (userAnswers["maxCholesterol"] == "") {
			userAnswers["maxCholesterol"] = "10000";
		}

		if (userAnswers["minSaturatedFat"] == "") {
			userAnswers["minSaturatedFat"] = "0";
		}
		if (userAnswers["maxSaturatedFat"] == "") {
			userAnswers["maxSaturatedFat"] = "10000";
		}

		if (userAnswers["minFiber"] == "") {
			userAnswers["minFiber"] = "0";
		}
		if (userAnswers["maxFiber"] == "") {
			userAnswers["maxFiber"] = "10000";
		}

		if (userAnswers["minSodium"] == "") {
			userAnswers["minSodium"] = "0";
		}
		if (userAnswers["maxSodium"] == "") {
			userAnswers["maxSodium"] = "10000";
		}

		if (userAnswers["minSugar"] == "") {
			userAnswers["minSugar"] = "0";
		}
		if (userAnswers["maxSugar"] == "") {
			userAnswers["maxSugar"] = "10000";
		}
	}
	// returns the map value with the given key 
	string accessMap(string key) {
		return userAnswers[key];
	}
	// returns the current offset
	size_t returnOffset() {
		return offset;
	}
	Recipe highestCal() {
		Recipe* temp = recipes[0];
		for (int i = 0; i < recipeCount - 1; i++) {
			if (temp->nutrition.calories < recipes[i + 1]->nutrition.calories)
				temp = recipes[i + 1];
		}
		return *temp;
	}
	// display all valid sorting options and gets user answer
	string sortingOptions() {
		string sorting;
		cout << "\n-----Sorting Options-----\nLikes, ReadyTime, Calories, Protein, Fat, Carbs\nCholesterol, SaturatedFats, Fiber, Sodium, Sugar" << endl;
		cout << "How would you like the results to be sorted (leave blank for unsorted): ";
		getline(cin, sorting, '\n');
		return convertLower(sorting);
	}
	// gets desired direction of sorting - default to descending
	bool sortingDirection(string sorting) {
		string input;
		if (sorting != "") {
			cout << "\n-----Sorting Direction-----\nAscending, Descending" << endl;
			cout << "Should the sorting be in ascending or descending order (leave blank for descending): ";
			getline(cin, input, '\n');
			input = convertLower(input);
			if (input == "ascending")
				return true;
			else
				return false;
		}
		return false;
	}
	// converts given string to lowercase
	string convertLower(string s) {
		transform(s.begin(), s.end(), s.begin(), tolower);
		return s;
	}
	// overloading operator[] to return recipe in desired position 
	const Recipe& operator[](const int i) {
		return *getRecipe(i);
	}
	// overloading operator++ to cycle all recipes up by one - first position goes to the back
	Recipe operator++(int i) {
		// loops through all recipes swapping them
		for (int x = 0; x < recipeCount - 1; x++) {
			swap(recipes[x], recipes[x + 1]);
		}
		return **recipes;
	}
protected:
	// collects all data from the api
	virtual void Data(const char arrData[], const unsigned int iSize) {
		jsonData.insert(jsonData.end(), arrData, arrData + iSize);
	}
	// called at the start of data collection
	virtual void StartOfData() {
		//cout << "Start of data" << endl;
	}
	// called at the end of data collection
	virtual void EndOfData() {
		const json& jf = json::parse(jsonData);
		parse(jf);
	}
private:
	// storage of all user answers for their custom query
	map<string, string> userAnswers;
	// storage of all data from the API or file
	vector<char> jsonData;
	// max allowed recipes 
	size_t maxRecipes{};
	// amount of recipes recieved from API
	size_t recipeCount{};
	// total amount of recipes matching the user query
	size_t totalResults{};
	// current offset of results
	size_t offset{};
	// storage of all recipes
	Recipe** recipes{};

	// parses the desired information out of the json object and stores it in an array of pointers
	void parse(const json& jf) {
		recipeCount = jf["results"].size();
		totalResults = jf["totalResults"];
		offset = jf["offset"];
		// loops through all recipes
		for (int i = 0; i < recipeCount; i++) {
			recipes[i] = new Recipe;
			recipes[i]->id = jf["results"][i]["id"];
			recipes[i]->title = jf["results"][i]["title"];
			recipes[i]->summary = jf["results"][i]["summary"];
			recipes[i]->likes = jf["results"][i]["aggregateLikes"];
			recipes[i]->readyTime = jf["results"][i]["readyInMinutes"];
			recipes[i]->sourceUrl = jf["results"][i]["sourceUrl"];
			recipes[i]->image = jf["results"][i]["image"];
			// loops through all cuisines
			size_t cuisineCount = jf["results"][i]["cuisines"].size();
			for (int x = 0; x < cuisineCount; x++) {
				string cuisine = jf["results"][i]["cuisines"][x];
				recipes[i]->cuisines.insert(recipes[i]->cuisines.end(), cuisine);
			}
			// loops through all dish types
			size_t dishTypeCount = jf["results"][i]["dishTypes"].size();
			for (int x = 0; x < dishTypeCount; x++) {
				string dishType = jf["results"][i]["dishTypes"][x];
				recipes[i]->dishType.insert(recipes[i]->dishType.end(), dishType);
			}
			// loops through all diets
			size_t dietCount = jf["results"][i]["diets"].size();
			for (int x = 0; x < dietCount; x++) {
				string diet = jf["results"][i]["diets"][x];
				recipes[i]->diets.insert(recipes[i]->diets.end(), diet);
			}
			recipes[i]->nutrition.calories = jf["results"][i]["nutrition"]["nutrients"][0]["amount"];
			recipes[i]->nutrition.protein = jf["results"][i]["nutrition"]["nutrients"][1]["amount"];
			recipes[i]->nutrition.fat = jf["results"][i]["nutrition"]["nutrients"][2]["amount"];
			recipes[i]->nutrition.carbs = jf["results"][i]["nutrition"]["nutrients"][3]["amount"];
			recipes[i]->nutrition.cholesterol = jf["results"][i]["nutrition"]["nutrients"][4]["amount"];
			recipes[i]->nutrition.satFats = jf["results"][i]["nutrition"]["nutrients"][5]["amount"];
			recipes[i]->nutrition.fiber = jf["results"][i]["nutrition"]["nutrients"][6]["amount"];
			recipes[i]->nutrition.sodium = jf["results"][i]["nutrition"]["nutrients"][7]["amount"];
			recipes[i]->nutrition.sugar = jf["results"][i]["nutrition"]["nutrients"][8]["amount"];
		}
	}
	// fixes the summary removing all of the recipes removing the <b>, </b>, </a>, and <a href: links
	void fixSummary() {
		// loops through all recipes
		for (int i = 0; i < recipeCount; i++) {
			size_t summaryLength = recipes[i]->summary.size();
			// loops through the entire summary removing the unwanted syntax - when they are removed also adjust the summary length to reflect the deletion
			for (int x = 0; x < summaryLength - 1; x++) {
				char current = recipes[i]->summary[x];
				char next = recipes[i]->summary[x + 1];
				char twoForward = recipes[i]->summary[x + 2];
				if ((current == '<' && twoForward == 'a') || (current == '<' && twoForward == 'b') || (current == '<' && twoForward == '>')) {
					if (twoForward == 'a' || twoForward == 'b') {
						recipes[i]->summary.erase(x, 4);
						summaryLength -= 4;
					}
					else {
						recipes[i]->summary.erase(x, 3);
						summaryLength -= 3;
					}
				}
				// when deleting the <a href: links loops till finding '>' counting each space
				if (current == '<' && next == 'a') {
					int posTemp = x;
					char charTemp = recipes[i]->summary[posTemp];
					while (charTemp != '>') {
						posTemp++;
						charTemp = recipes[i]->summary[posTemp];
					}
					int difference = posTemp - x;
					recipes[i]->summary.erase(x, difference + 1);
					summaryLength -= difference + 1;
				}
			}
		}
	}
	// deletes data of previous API call to prepare the storage/containers for the next call
	void DeleteMemory() {
		// clear char vector
		jsonData.clear();
		// loops through each occupied recipe in the recipes array deleting them 
		for (int i = 0; i < recipeCount; i++)
			delete recipes[i];
		// sets the recipes back to null
		for (int i = 0; i < recipeCount; i++)
				recipes[i] = nullptr;
	}
};
// Compares the likes of two recipes
struct CompareLikes {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->likes > p2->likes)
			return true;
		else
			return false;
	};
};
// Compares the ready time of two recipes
struct CompareReadyTime {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->readyTime > p2->readyTime)
			return true;
		else
			return false;
	};
};
// Compares the calories of two recipes
struct CompareCalories {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.calories > p2->nutrition.calories)
			return true;
		else
			return false;
	};
};
// Compares the protein of two recipes
struct CompareProtein {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.protein > p2->nutrition.protein)
			return true;
		else
			return false;
	};
};
// Compares the fat of two recipes
struct CompareFat {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.fat > p2->nutrition.fat)
			return true;
		else
			return false;
	};
};
// Compares the carbs of two recipes
struct CompareCarbs {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.carbs > p2->nutrition.carbs)
			return true;
		else
			return false;
	};
};
// Compares the cholesterol of two recipes
struct CompareCholesterol {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.cholesterol > p2->nutrition.cholesterol)
			return true;
		else
			return false;
	};
};
// Compares the saturated fat of two recipes
struct CompareSatFats {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.satFats > p2->nutrition.satFats)
			return true;
		else
			return false;
	};
};
// Compares the fiber of two recipes
struct CompareFiber {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.fiber > p2->nutrition.fiber)
			return true;
		else
			return false;
	};
};
// Compares the sodium of two recipes
struct CompareSodium {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.sodium > p2->nutrition.sodium)
			return true;
		else
			return false;
	};
};
// Compares the sugar of two recipes
struct CompareSugar {
	bool Compare(RecipeAPI::Recipe* p1, RecipeAPI::Recipe* p2) {
		if (p1->nutrition.sugar > p2->nutrition.sugar)
			return true;
		else
			return false;
	};
};
// overloaded operator<< to display the information of a RecipeAPI object
ostream& operator<<(ostream& cout, RecipeAPI& recipes) {
	recipes.displayRecipes(cout);
	return cout;
}
// overloaded operator<< to display the information of a RecipeAPI::Recipe object 
ostream& operator<<(ostream& cout, RecipeAPI::Recipe recipe) {
	RecipeAPI r;
	cout << "\tID: " << recipe.id << "\t\tTitle: " << recipe.title << "\t\tCook time: " << recipe.readyTime << " minutes\t\tLikes: " << recipe.likes << endl;
	cout << "\tAll Cuisines: " << r.getAllCuisines(recipe) << "\n\tAll Diets: " << r.getAllDiets(recipe) << "\n\tAll Dish Types: " << r.getAllDishTypes(recipe) << endl;
	cout << "\t----------------------------  Nutrition  ----------------------------" << r.getAllNutrition(recipe) << endl;
	cout << "\t---------------------------------------------------------------------" << endl;
	cout << "\tSource website: " << recipe.sourceUrl << endl;
	cout << "\tSummary: " << recipe.summary << endl << endl;
	return cout;
}



// Tests and displays the information from TestData file recipeQueryOne.json - Uses config TESTING
#ifdef _TESTING
int main(int argc, char* argv[]) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	RecipeAPI r;
	r.openFile("recipeQueryOne.json");
	r.parse();
	r.displayStats();

	string sorting;
	bool ascending;
	sorting = r.sortingOptions();
	ascending = r.sortingDirection(sorting);

	if (sorting == "likes")
		r.bubbleSort<CompareLikes>(ascending);
	else if (sorting == "readytime")
		r.bubbleSort<CompareReadyTime>(ascending);
	else if (sorting == "calories")
		r.bubbleSort<CompareCalories>(ascending);
	else if (sorting == "protein")
		r.bubbleSort<CompareProtein>(ascending);
	else if (sorting == "fat")
		r.bubbleSort<CompareFat>(ascending);
	else if (sorting == "carbs")
		r.bubbleSort<CompareCarbs>(ascending);
	else if (sorting == "cholesterol")
		r.bubbleSort<CompareCholesterol>(ascending);
	else if (sorting == "saturatedfats")
		r.bubbleSort<CompareSatFats>(ascending);
	else if (sorting == "fiber")
		r.bubbleSort<CompareFiber>(ascending);
	else if (sorting == "sodium")
		r.bubbleSort<CompareSodium>(ascending);
	else if (sorting == "sugar")
		r.bubbleSort<CompareSugar>(ascending);

	// dispaly results in TestData\\results.txt
	r.resultsToFile();
	// display all recipes properly sorted
	cout << r << endl;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl << endl << endl;
	// display recipes with certain indexes 
	cout << "1)" << r[0];
	cout << "2)" << r[1] << endl;
	// display recipe with highest calories
	cout << "Highest Calorie Recipe)" << r.highestCal() << endl;
	// cycle all recipes up by 1 - top goes to the back
	r++;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl << endl << endl;
	// display all recipes now being shifted up by one
	cout << r << endl;
	string view;
	cout << "(yes/no) - Do you want to view one of these recipes? ";
	cin >> view;
	if (view == "yes")
		r.displayBrowser();
}
#endif
// Tests and displays the information from TestData file recipeQueryTwo.json - Uses config TESTINGTWO
#ifdef _TESTINGTWO
int main(int argc, char* argv[]) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	RecipeAPI r;
	r.openFile("recipeQueryTwo.json");
	r.parse();
	r.displayStats();
	

	string sorting;
	bool ascending;
	sorting = r.sortingOptions();
	ascending = r.sortingDirection(sorting);

	cout << r.highestCal();

	if (sorting == "likes")
		r.bubbleSort<CompareLikes>(ascending);
	else if (sorting == "readytime")
		r.bubbleSort<CompareReadyTime>(ascending);
	else if (sorting == "calories")
		r.bubbleSort<CompareCalories>(ascending);
	else if (sorting == "protein")
		r.bubbleSort<CompareProtein>(ascending);
	else if (sorting == "fat")
		r.bubbleSort<CompareFat>(ascending);
	else if (sorting == "carbs")
		r.bubbleSort<CompareCarbs>(ascending);
	else if (sorting == "cholesterol")
		r.bubbleSort<CompareCholesterol>(ascending);
	else if (sorting == "saturatedfats")
		r.bubbleSort<CompareSatFats>(ascending);
	else if (sorting == "fiber")
		r.bubbleSort<CompareFiber>(ascending);
	else if (sorting == "sodium")
		r.bubbleSort<CompareSodium>(ascending);
	else if (sorting == "sugar")
		r.bubbleSort<CompareSugar>(ascending);

	// dispaly results in TestData\\results.txt
	r.resultsToFile();
	// display all recipes properly sorted
	cout << r << endl;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl << endl << endl;
	// display recipes with certain indexes 
	cout << "1)" << r[0];
	cout << "5)" << r[4];
	cout << "10)" << r[9] << endl;
	// display recipe with highest calories
	cout << "Highest Calorie Recipe)" << r.highestCal() << endl;
	// cycle all recipes up by 1 three times - top goes to the back
	r++;
	r++;
	r++;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl;
	cout << "********************************************************************************************************************" << endl << endl << endl;
	// display all recipes now being shifted up by three
	cout << r << endl;
	string view;
	cout << "(yes/no) - Do you want to view one of these recipes? ";
	cin >> view;
	if (view == "yes")
		r.displayBrowser();
}
#endif
// Tests and displays the information from the API(spoonacular-recipe-food-nutrition-v1.p.rapidapi.com) using a custom query of the user  - Uses config Release
#ifdef _RELEASE
int main(int argc, char* argv[]) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	string offset = "0";
	string sorting;
	bool first = true;
	bool ascending;
	int input = 0;
	const int ARROW_UP = 72;
	const int ARROW_DOWN = 80;
	const int V_KEY = 118;

	RecipeAPI r;
	r.intro();
	r.getUserReq();
	sorting = r.sortingOptions();
	ascending = r.sortingDirection(sorting);
	r.Connect("spoonacular-recipe-food-nutrition-v1.p.rapidapi.com");
	do {
		offset = r.paginate(input, offset);
		r.Get("/recipes/complexSearch", { {"rapidapi-key", "5f9980dfd3msh9198d78edccd265p1a3014jsnf19da5ef8bb0"}, {"query", r.accessMap("query")}, {"cuisine", r.accessMap("cuisine")}, {"diet", r.accessMap("diet")}, {"intolerances", r.accessMap("intolerances")}, {"includeIngredients", r.accessMap("includeIngredients")},
		 {"excludeIngredients", r.accessMap("excludeIngredients")}, {"type", r.accessMap("type")}, {"maxReadyTime", r.accessMap("maxReadyTime")}, {"minCarbs", r.accessMap("minCarbs")}, {"maxCarbs", r.accessMap("maxCarbs")}, {"minProtein", r.accessMap("maxProtein")}, {"minCalories", r.accessMap("minCalories")},
		 {"maxCalories", r.accessMap("maxCalories")}, {"minFat", r.accessMap("minFat")}, {"maxFat", r.accessMap("maxFat")}, {"minCholesterol", r.accessMap("minCholesterol")}, {"maxCholesterol", r.accessMap("maxCholesterol")}, {"minSaturatedFat", r.accessMap("minSaturatedFat")}, {"maxSaturatedFat", r.accessMap("maxSaturatedFat")},
		 {"minFiber", r.accessMap("minFiber")}, {"maxFiber", r.accessMap("maxFiber")}, {"minSodium", r.accessMap("minSodium")}, {"maxSodium", r.accessMap("maxSodium")}, {"minSugar", r.accessMap("minSugar")}, {"maxSugar", r.accessMap("maxSugar")},{"addRecipeInformation", "true"}, {"offset", offset}, {"sort", "popularity"}, {"sortDirection", "desc"}, { "number", "10" }, {"ranking", "2"} });
		if (first == true)
			r.displayStats();

		if (sorting == "likes")
			r.bubbleSort<CompareLikes>(ascending);
		else if (sorting == "readytime")
			r.bubbleSort<CompareReadyTime>(ascending);
		else if (sorting == "calories")
			r.bubbleSort<CompareCalories>(ascending);
		else if (sorting == "protein")
			r.bubbleSort<CompareProtein>(ascending);
		else if (sorting == "fat")
			r.bubbleSort<CompareFat>(ascending);
		else if (sorting == "carbs")
			r.bubbleSort<CompareCarbs>(ascending);
		else if (sorting == "cholesterol")
			r.bubbleSort<CompareCholesterol>(ascending);
		else if (sorting == "saturatedfats")
			r.bubbleSort<CompareSatFats>(ascending);
		else if (sorting == "fiber")
			r.bubbleSort<CompareFiber>(ascending);
		else if (sorting == "sodium")
			r.bubbleSort<CompareSodium>(ascending);
		else if (sorting == "sugar")
			r.bubbleSort<CompareSugar>(ascending);

		r.resultsToFile();
		cout << r << endl;

		input = r.continueMessage();
		if (input == V_KEY) {
			r.displayBrowser();
			input = r.continueMessage();
		}

		offset = to_string(r.returnOffset());
		first = false;
	} while (input == ARROW_UP || input == ARROW_DOWN);
}
#endif