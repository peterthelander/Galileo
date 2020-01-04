
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <cstdio>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::gregorian;
using boost::lexical_cast;

typedef boost::tokenizer<boost::char_separator<char> > tok;

vector<date> dateGuide;

vector<string> asxNames;
vector<string> usNames;

const size_t UNDEFINED = 999999;

void generateDateGuide()
{
	for (date d(2007,Jan,29); d <= date(2009,Dec,31);)
	{
		dateGuide.push_back(d);

		if (d.day_of_week() == Friday)
			d += days(3);
		else
			d += days(1);
	}

	cout << "Dates from " << dateGuide[0] << " to " << dateGuide.back() ;
	cout << ", " << dateGuide.size() << " days." << endl;
}

struct Stock
{
	float open;
	float close;
	float move;
};

void setMove(Stock* s)
{
	if (s->open != 0 && s->close != 0)
		s->move = (s->close / s->open - 1) *100;
	else
		s->move = 0;

	if (fabs(s->move) > 15)
		int x=1;
}

vector<Stock*> us;
vector<Stock*> asx;

string loadFile(path name)
{
	size_t sz = (size_t)file_size(name);

	char * buffer = new char[sz+1];

	ifstream fs(name.string().c_str());
	fs.get(buffer, (streamsize)sz, 0);
	fs.close();
	buffer[sz] = 0;

	string result = buffer;

	delete buffer;

	return result;
}

vector<string> splitString(const string& input, const string& separators)
{
	vector<string> result;

	tok tokens(input, char_separator<char>(separators.c_str()));

	for (tok::iterator ti = tokens.begin();
		ti != tok::iterator();
		ti++)
	{
		result.push_back(*ti);
	}

	return result;
}

void splitString2(string& input, const string& separators, vector<string>& result )
{
	//if (input.size() > 450000)
	//	input = (input.substr(0, 50000) + input.substr(input.size()-50000));

	tok tokens(input, char_separator<char>(separators.c_str()));

	for (tok::iterator ti = tokens.begin();
		ti != tok::iterator();
		ti++)
	{
		result.push_back(*ti);
	}
}

void summarizeDirectory(path p)
{
	path input = current_path() / p;
	if (!exists(input))
		input = current_path() / "../debug" / p;

	assert(exists(input));

	cout << "symbol,numDays,startDate,endDate,avgAbsMove,avgVol,avgVolAmt" << endl;

	bool isAsx = (p.string().find("asx") != string::npos);
	
	for (directory_iterator di(input); 
		 di != directory_iterator();
		 di++)
	{
		string baseName = di->path().leaf();
		string stockSymbol = splitString(baseName, ".")[0];

		if (baseName.find(".CSV") == string::npos &&
			baseName.find(".ASC") == string::npos)
			continue;

		if (isAsx && stockSymbol.size() != 3)
			continue;

		string fileContents = loadFile(di->path());

		vector<string> lines = splitString(fileContents, "\n");

		unsigned int num = 0;
		double totalAbsMove = 0;
		double totalVolume = 0;
		double totalVolAmt = 0;
		unsigned long startDate = 0;
		unsigned long endDate = 0;

		for (size_t i=0; i<lines.size(); i++)
		{
			string line = lines[i];

			vector<string> fields = splitString(line, ",");

			unsigned long dateLong = 0;
			float open = 0;
			float high = 0;
			float low = 0;
			float close = 0;
			unsigned long volume = 0;

			try
			{
				if (fields[1].find("/") == string::npos)
				{
					dateLong = lexical_cast<unsigned long>(fields[1]);
					open = lexical_cast<float>(fields[2]);
					high = lexical_cast<float>(fields[3]);
					low = lexical_cast<float>(fields[4]);
					close = lexical_cast<float>(fields[5]);
					volume = lexical_cast<unsigned long>(fields[6]);
				}
				else
				{
					string dateString = fields[0];
					vector<string> dateParts = splitString(dateString, "/");
					unsigned long month = lexical_cast<unsigned long>(dateParts[0]);
					unsigned long day = lexical_cast<unsigned long>(dateParts[1]);
					unsigned long year = lexical_cast<unsigned long>(dateParts[2]);
					dateLong = year * 10000 + month * 100 + day;

					open = lexical_cast<float>(fields[1]);
					high = lexical_cast<float>(fields[2]);
					low = lexical_cast<float>(fields[3]);
					close = lexical_cast<float>(fields[4]);
					volume = lexical_cast<unsigned long>(fields[5]);
				}
			}
			catch (const bad_lexical_cast& b)
			{
				cout << "Caught bad_lexical_cast exception: \"" << b.what() << "\"" << endl;
			}

			if (dateLong != 0)
			{
				if (startDate == 0 || dateLong < startDate)
					startDate = dateLong;

				if (endDate == 0 || dateLong > endDate)
					endDate = dateLong;
			}

			float move = 0;

			if (open != 0)
				move = ((close/open)-1)*100;

			totalAbsMove += abs(move);
			totalVolume += volume;
			totalVolAmt += volume*(open+close)/2;

			num ++;
		}

		double avgAbsMove = totalAbsMove / num;
		double avgVolume = totalVolume / num;
		double avgVolAmt = totalVolAmt / num;

		cout << stockSymbol;
		cout << "," << num;
		cout << "," << startDate;
		cout << "," << endDate;
		cout << "," << avgAbsMove;
		cout << "," << avgVolume;
		cout << "," << avgVolAmt << endl;

	}
}

bool isAsx;

void loadTextSaveDataFile(path p)
{
	string nm = p.leaf();
//	if (nm.find("_") != string::npos)
//		return;

	cout << p.leaf() << endl;

	Stock * stock = new Stock[dateGuide.size()];

	size_t dateIndex = 0;

	for (dateIndex = 0; dateIndex < dateGuide.size(); dateIndex++)
	{
		stock[dateIndex].open = 0;
		stock[dateIndex].close = 0;
		stock[dateIndex].move = 0;
	}

	dateIndex = 0;

	string fileContents = loadFile(p);

	vector<string> lines = splitString(fileContents, "\n");

	for (size_t i=0; i<lines.size(); i++)
	{
		string line = lines[i];

		if (line.find("Date,") != string::npos)
			continue;

		vector<string> fields = splitString(line, ",");

		unsigned short year, month, day;
		float open = 0;
		float high = 0;
		float low = 0;
		float close = 0;
		unsigned long volume = 0;

		try
		{
			int p = 0;
			char c = line[0];
			if (c >= '0' && c <= '9')
				p = 0;
			else
				p = 1;

			string sepStr;

			if (fields[p].find("/") != string::npos)
				sepStr = "/";
			else if (fields[p].find("-") != string::npos)
				sepStr = "-";

			if (!sepStr.empty())
			{
				string dateString = fields[p++];
				vector<string> dateParts = splitString(dateString, sepStr);
				if (lexical_cast<unsigned short>(dateParts[0]) > 12)
				{
					year = lexical_cast<unsigned short>(dateParts[0]);
					month = lexical_cast<unsigned short>(dateParts[1]);
					day = lexical_cast<unsigned short>(dateParts[2]);
				}
				else
				{
					month = lexical_cast<unsigned short>(dateParts[0]);
					day = lexical_cast<unsigned short>(dateParts[1]);
					year = lexical_cast<unsigned short>(dateParts[2]);
				}

				open = lexical_cast<float>(fields[p++]);
				high = lexical_cast<float>(fields[p++]);
				low = lexical_cast<float>(fields[p++]);
				close = lexical_cast<float>(fields[p++]);
				volume = lexical_cast<unsigned long>(fields[p++]);
			}
			else
			{
				unsigned long dateLong = lexical_cast<unsigned long>(fields[p++]);
				year = (unsigned short)(dateLong / 10000);
				month = (unsigned short)((dateLong / 100)%100);
				day = (unsigned short)(dateLong % 100);
				open = lexical_cast<float>(fields[p++]);
				high = lexical_cast<float>(fields[p++]);
				low = lexical_cast<float>(fields[p++]);
				close = lexical_cast<float>(fields[p++]);
				volume = lexical_cast<unsigned long>(fields[p++]);
			}
		}
		catch (const bad_lexical_cast& b)
		{
			cout << "Caught bad_lexical_cast exception: \"" << b.what() << "\"" << endl;
		}

		float move = 0;

		if (open != 0)
			move = ((close/open)-1)*100;

		date d(year, month, day);

		while (d > dateGuide[dateIndex])
		{
			stock[dateIndex].open = 0;
			stock[dateIndex].close = 0;
			stock[dateIndex].move = 0;
			dateIndex ++;
		}

		if (d < dateGuide[dateIndex])
			continue;

		assert(d == dateGuide[dateIndex]);

		stock[dateIndex].open = open;
		stock[dateIndex].close = close;
		stock[dateIndex].move = move;
		//dateIndex ++; // if going forward in dates
		dateIndex = 0; // if going backwards

	}

	// save

	string inFileName = p.string();
	string outFileName = inFileName.substr(0, inFileName.size()-4) + ".DAT";

	FILE *fp;
	fopen_s(&fp, outFileName.c_str(), "wb");
	fwrite((const char*)stock, sizeof(Stock), dateGuide.size(), fp);
	fclose(fp);

	delete stock;
}

void loadTextSaveData(path p)
{
	path input = current_path() / p;
	if (!exists(input))
		input = current_path() / "../debug" / p;

	assert(exists(input));

	isAsx = (p.string().find("asx") != string::npos);

	for (directory_iterator di(input); 
		 di != directory_iterator();
		 di++)
	{
		string baseName = di->path().leaf();

		if (baseName.find(".CSV") == string::npos &&
			baseName.find(".csv") == string::npos &&
			baseName.find(".ASC") == string::npos)
			continue;

		string stockSymbol = splitString(baseName, ".")[0];

		if (isAsx && stockSymbol.size() != 3)
			continue;

		loadTextSaveDataFile(di->path());
	}
}

void loadTextWeblinkSaveData(path p)
{
	path input = current_path() / p;
	if (!exists(input))
		input = current_path() / "../debug" / p;

	assert(exists(input));

	isAsx = (p.string().find("asx") != string::npos);

	string asxIndexCodes = "DEFHIMNSTU";

	for (size_t s=0; s<asxIndexCodes.size(); s++)
	{
		Stock * p = new Stock[dateGuide.size()];
		for (size_t d=0; d<dateGuide.size(); d++)
		{
			p[d].open = 0;
			p[d].close = 0;
			p[d].move = 0;
		}
		asx.push_back(p);
	}

	for (directory_iterator di(input); 
		 di != directory_iterator();
		 di++)
	{
		string baseName = di->path().leaf();

		if (baseName.find(".CSV") == string::npos &&
			baseName.find(".csv") == string::npos &&
			baseName.find(".ASC") == string::npos)
			continue;

		string dateString = splitString(baseName, ".")[0];
		cout << dateString;

		int year = lexical_cast<int>(dateString.substr(0,4));
		int month = lexical_cast<int>(dateString.substr(4,2));
		int day = lexical_cast<int>(dateString.substr(6,2));

		date dt(year, month, day);

		size_t d = 0;
		for (; d<dateGuide.size(); d++)
		{
			if (dateGuide[d] == dt)
				break;
		}

		if (d == dateGuide.size())
			continue;

		string fileContents = loadFile(di->path());

		cout << ".";

		vector<string> lines;
		
		splitString2(fileContents, "\n", lines);

		cout << ".";

		for (size_t i=0; i<lines.size(); i++)
		{
			string line = lines[i];

			if (line[0] != 'X')
				continue;

			if (line.size() < 3 || line[2] != 'J')
				continue;

			size_t indexNum = asxIndexCodes.find(line[1]);

			if (indexNum == string::npos)
				continue;

			Stock * stocks = asx[indexNum];

			vector<string> fields = splitString(line, ",");

			if (fields.size() != 4)
				continue;

			Stock& stock = stocks[d];

			string timeOfDay = fields[2];
			string value = fields[3];

			if (stock.open == 0 && timeOfDay.substr(0,4) >= "101600")
			{
				stock.open = boost::lexical_cast<float>(value);
			}

			if (stock.close == 0 && timeOfDay >= "154500")
			{
				stock.close = boost::lexical_cast<float>(value);

				assert(stock.open != 0);

				setMove(&stock);
			}
		}
		cout << ".";
	}

	for (size_t s=0; s<asxIndexCodes.size(); s++)
	{
		string name = "X";
		name += asxIndexCodes.substr(s,1);
		name += "J";

		asxNames.push_back(name);

		path outFileName = input / (name+".DAT");

		FILE *fp;
		fopen_s(&fp, outFileName.string().c_str(), "wb");
		fwrite((const char*)asx[s], sizeof(Stock), dateGuide.size(), fp);
		fclose(fp);
	}
}

string prophetToYahoo(string symbol)
{
	
	if (symbol[0] == '_')
	{
		string suffix = symbol.substr(1);
		/*
		if (suffix == "COMP")        suffix = "DJA";
		else if (suffix == "COMPQ")  suffix = "IXIC";
		else if (suffix == "INDU")   suffix = "DJI";
		else if (suffix == "TRANA")  suffix = "DJT";
		else if (suffix == "UTIL")   suffix = "DJU";
		*/
		symbol = (string)"^" + suffix;
	}

	if (symbol.size() == 6 && symbol[0] == 'X' && symbol.substr(3) == ".AX")
		symbol = "^A"+symbol.substr(0,3);

	return symbol;
}

void downloadStockData(string symbol, date start, date end)
{
	char buf[1024];

	int b = start.day();

	symbol = prophetToYahoo(symbol);

	sprintf_s(buf, 
		"curl -o update.csv \"http://ichart.finance.yahoo.com/table.csv?"
			"a=%d&b=%d&c=%d&d=%d&e=%d&f=%d&s=%s&y=0&g=d&ignore=.csv\"",
		(int)start.month()-1,
		(int)start.day(),
		(int)start.year(),
		(int)end.month()-1,
		(int)end.day(),
		(int)end.year(),
		symbol.c_str()
	);

	//sprintf_s(buf, "echo hello > file.txt", "");

	cout << buf << endl;
	
	system(buf);

// eg:
// curl -o msft.csv http://table.finance.yahoo.com/table.csv?
//     a=9&b=24&c=1970&d=9&e=30&f=2002&s=msft&y=0&g=d&ignore=.csv

}

void determineStartEnd(Stock* stock, size_t& start, size_t& end)
{
	start = dateGuide.size();
	end = -1;

	for (size_t i = 0; i < dateGuide.size(); i++)
	{
		if (stock[i].open != 0)
		{
			if (start == dateGuide.size())
				start = i;
			end = i;
		}
	}
}

bool liveStock;

size_t dateToDateGuideIndex(date d)
{
	bool found = false;
	size_t s = 0;
	for (; s<dateGuide.size(); s++)
	{
		found = (dateGuide[s] == d);
		if (found)
			break;
	}
	if (!found)
		s = UNDEFINED;

	return s;
}

void loadFromUpdate(Stock* stock)
{
	liveStock = true;

	path input = current_path() / "update.csv";
	if (!exists(input))
		input = current_path() / "../debug" / "update.csv";

	assert(exists(input));

	size_t dateIndex = 0;

	string fileContents = loadFile(input);

	vector<string> lines = splitString(fileContents, "\n");

	for (size_t i=0; i<lines.size(); i++)
	{
		string line = lines[i];

		if (line.find("Date,") != string::npos)
			continue;

		vector<string> fields = splitString(line, ",");

		unsigned short year, month, day;
		float open = 0;
		float high = 0;
		float low = 0;
		float close = 0;
		unsigned long volume = 0;

		try
		{
			string dateString = fields[0];
			vector<string> dateParts = splitString(dateString, "-");
			year = lexical_cast<unsigned short>(dateParts[0]);
			month = lexical_cast<unsigned short>(dateParts[1]);
			day = lexical_cast<unsigned short>(dateParts[2]);
			open = lexical_cast<float>(fields[1]);
			high = lexical_cast<float>(fields[2]);
			low = lexical_cast<float>(fields[3]);
			close = lexical_cast<float>(fields[4]);
		}
		catch (const bad_lexical_cast& b)
		{
			cout << "Caught bad_lexical_cast exception: \"" << b.what() << "\"" << endl;
			liveStock = false;
			return;
		}

		float move = 0;

		if (open != 0)
			move = ((close/open)-1)*100;

		date d(year, month, day);

		size_t s = dateToDateGuideIndex(d);

		if (s != UNDEFINED)
		{
			stock[s].open = open;
			stock[s].close = close;
			stock[s].move = move;
		}
	}
}

string findString(vector<string> haystack, string needle)
{
	for (size_t l=0; l<haystack.size(); l++)
		if (haystack[l].find(needle) != string::npos)
			return haystack[l];
	return "";
}

bool getOvernightValue(string symbol, Stock* stock)
{
	date lastWeekday = day_clock::local_day() - days(1);

	if (lastWeekday.day_of_week() == Sunday)
		lastWeekday -= days(2);
	else if (lastWeekday.day_of_week() == Saturday)
		lastWeekday -= days(1);

	size_t d = 0;
	for (; d<dateGuide.size(); d++)
	{
		if (dateGuide[d] == lastWeekday)
			break;
	}

	assert(d != dateGuide.size());

	// maybe already downloaded it?
	if (stock[d].move != 0)
		return false;

	float previous = stock[d-1].close;

	if (previous == 0.0)
		previous = stock[d-2].close;
	if (previous == 0.0)
		previous = stock[d-3].close;

	char buf[1024];

	// get us value of yesterday, from the main page

	symbol = prophetToYahoo(symbol);

	string url = "http://finance.yahoo.com/q?s=" + symbol;

	sprintf_s(buf, "curl -o mainpage.html \"%s\"", url.c_str());

	//sprintf_s(buf, "echo hello > file.txt", "");

	cout << buf << endl;
	
	system(buf);

	string fileContents = loadFile("mainpage.html");

	vector<string> lines = splitString(fileContents, "\n");

	string line = findString(lines, ">Index Value:<");
	
	vector<string> peices = splitString(line, "><");

	// now find string which begins with " (" and ends with "%)"

	float val = 0;
	
	for (size_t l=0; l<peices.size(); l++)
	{
		float v = 0;

		char buf[10000];
		int pos = 0;
		for (size_t cn=0; cn<peices[l].size(); cn++)
			if (peices[l][cn] != ',')
				buf[pos++] = peices[l][cn];
		buf[pos] = 0;
			
		string commalessPeice = buf;

		try
		{
			v = lexical_cast<float>(commalessPeice);
		}
		catch (const bad_lexical_cast& )
		{
			v = 0;
			continue;
		};

		if (v > previous*0.9 && v < previous * 1.1)
		{
			val = v;
			assert(val != 0.0);
			break;
		}
	}

	if (val == 0 || previous == 0)
		stock[d].move = 0;
	else
		stock[d].move = (val/previous - 1)*100;

	return true;
}

bool checkForUpdates(string symbol, Stock* stock)
{
	bool result = false;
	size_t start, end;
	determineStartEnd(stock, start, end);

	// now end is the highest date with a non-zero move

	size_t firstUndefined = end+1;

	date firstUndefinedDate = dateGuide[firstUndefined];

	// should be able to download data up until yesterday
	// so first undefined date must be at least today

	date today = day_clock::local_day();

	liveStock = true;

	if (firstUndefinedDate < today - days(1))
	{
		cout << "Need to update from " << firstUndefinedDate << endl;

		downloadStockData(symbol, firstUndefinedDate, today);
		//downloadStockData(symbol, date(2002, Jan, 1), date(2002, Dec, 31));

		if (liveStock == false)
			return true;

		loadFromUpdate(stock);

		result = true;
	}

	return result;
}

Stock* loadDataStock(path p)
{
	Stock * stock = new Stock[dateGuide.size()];
	//Stock r[5219];
	//Stock * result = &r[0];

	for (size_t i = 0; i< dateGuide.size(); i++)
	{
		stock[i].open = 0;
		stock[i].close = 0;
		stock[i].move = 0;
	}

	string filename = p.string();

	FILE *fp;
	fopen_s(&fp, filename.c_str(), "rb");
	fread(&stock[0], sizeof(Stock), dateGuide.size(), fp);
	fclose(fp);

	string s = p.leaf();

	vector<string> parts = splitString(s, ".");
	string symbol = parts[0];

	bool isAsx = false;

	if (filename.find("/asx/") != string::npos)
	{
		isAsx = true;
		symbol=symbol+".AX";
	}

	bool changed = false;

	if (isAsx)
	{
		// TODO: updating for asx stocks?
	}
	else
	{
		if (checkForUpdates(symbol, stock))
			changed = true;
		
		if (getOvernightValue(symbol, stock))
			changed = true;
	}

	if (changed)
	{
		FILE *fp;
		fopen_s(&fp, filename.c_str(), "wb");
		fwrite((const char*)stock, sizeof(Stock), dateGuide.size(), fp);
		fclose(fp);
	}

	return stock;
}

vector<Stock*> loadDataStocks(path p)
{
	vector<Stock*> result;

	path input = current_path() / p;
	if (!exists(input))
		input = current_path() / "../debug" / p;

	bool isAsx = (input.string().find("asx")!=string::npos);

	assert(exists(input));

	cout << "Loading data: " << input << ": " << endl;

	for (directory_iterator di(input); 
		 di != directory_iterator();
		 di++)
	{
		string baseName = di->path().leaf();

		if (baseName.substr(baseName.size()-4) != ".DAT")
			continue;

		// from us, ignore non-indices
		if (di->path().string().find("us/") != string::npos && 
			di->path().string().find("_") == string::npos)
			continue;

		vector<string> nameParts = splitString(baseName, ".");
		
		if (isAsx)
			asxNames.push_back(nameParts[0]);
		else
			usNames.push_back(nameParts[0]);

		cout << baseName << " ";

		result.push_back(loadDataStock(di->path()));

		size_t start, end;
		determineStartEnd(result.back(), start, end);

		unsigned int numNonZero = 0;
		for (size_t d=0; d<end; d++)
		{
			if (result.back()[d].move != 0)
				numNonZero++;

			//if (result.back()[d].move == 0)
			//	cout << dateGuide[d] << endl;
		}

		float moveness = (float)numNonZero / (end-start);

		cout << dateGuide[start] << " " << dateGuide[end] << " " << moveness << endl;

		if (moveness < 0.8)
		{
			path p = di->path();
			string newname = p.string() + ".dead";
			rename(p, newname);
		}
	}

	cout << endl;

	return result;
}

double correlation(Stock* usStock, Stock* asxStock)
{
	Stock * x = usStock;
	Stock * y = &asxStock[1];
	size_t N = dateGuide.size() - 1;

	// first, find common

	while (x->move == 0 || y->move == 0)
	{
		x++;
		y++;
		N--;
	}

	while (x[N-1].move == 0 || y[N-1].move == 0)
		N--;

	// correlating set of N pairs
	// us values from 0..N-2 against asx values from 1..N-1

	double sum_sq_x = 0;
	double sum_sq_y = 0;
	double sum_coproduct = 0;
	double mean_x = x[0].move;
	double mean_y = x[0].move;
	for (size_t i=2; i <= N; i++)
	{
		double sweep = (double)((double)i - 1.0) / i;

		if (x[i-1].move != 0)
		{
			int x=1;
		}
		double delta_x = x[i-1].move - mean_x;
		double delta_y = y[i-1].move - mean_y;
		sum_sq_x += delta_x * delta_x * sweep;
		sum_sq_y += delta_y * delta_y * sweep;
		sum_coproduct += delta_x * delta_y * sweep;
		mean_x += delta_x / i;
		mean_y += delta_y / i;
	}
	double pop_sd_x = sqrt( sum_sq_x / N );
	double pop_sd_y = sqrt( sum_sq_y / N );
	double cov_x_y = sum_coproduct / N;
	double correlation = cov_x_y / (pop_sd_x * pop_sd_y);

	return correlation;
}

typedef vector<vector<double> > Matrix; // if matrix is vertical, then m.size()==1

double correlation(Matrix& X, Matrix& Y)
{
	double sum_sq_x = 0.0f;
	double sum_sq_y = 0.0f;
	double sum_coproduct = 0.0f;
	double mean_x = X[0][0];
	double mean_y = Y[0][0];
	size_t N = X[0].size();
	for (size_t i = 2; i<=N; i++)
	{
		double sweep = (double)((double)i - 1.0) / i;
		double delta_x = X[0][i-1] - mean_x;
		double delta_y = Y[0][i-1] - mean_y;
		sum_sq_x += delta_x * delta_x * sweep;
		sum_sq_y += delta_y * delta_y * sweep;
		sum_coproduct += delta_x * delta_y * sweep;
		mean_x += delta_x / i;
		mean_y += delta_y / i;
	}
	double pop_sd_x = sqrt( sum_sq_x / N );
	double pop_sd_y = sqrt( sum_sq_y / N );
	double cov_x_y = sum_coproduct / N;
	double correlation = cov_x_y / (pop_sd_x * pop_sd_y);
	return correlation;
}

/*
void correlationGrid()
{
	for (size_t asxIndex = 0; asxIndex < asx.size(); asxIndex++)
	{
		cout << asxNames[asxIndex] << " ";
		for (size_t usIndex = 0; usIndex < us.size(); usIndex++)
		{
			Stock * usAvg = new Stock[dateGuide.size()];

			double r = correlation(us[usIndex], asx[asxIndex]);

			int r2 = (int)(r*100);

			if (abs(r2) < 5)
			{
				cout << "  ";
			}
			else
			{
				cout << r2;
				if (abs(r2) > 20)
					cout << usNames[usIndex];
			}
		}
		cout << endl;
	}

}
*/

void transpose(const Matrix& in, Matrix& out)
{
	out.clear();

	for (size_t x = 0; x < in.size(); x++)
	{
		for (size_t y = 0; y < in[0].size(); y++)
		{
			if (x==0)
				out.push_back(vector<double>());

			out[y].push_back(in[x][y]);
		}
	}
}

void multiply(const Matrix &left, const Matrix& right, Matrix& out)
{
	// left width x left height
	// right width x right height

	// result has: left height, right width

	out.clear();

	assert(left.size() == right[0].size());

	for (size_t outx = 0; outx < right.size(); outx++)
	{
		out.push_back(vector<double>());
		for (size_t outy = 0; outy < left[0].size(); outy++)
		{
			double subtotal = 0.0;
			for (size_t i = 0; i < left.size(); i++)
			{
				subtotal += left[i][outy] * right[outx][i];
			}
			out[outx].push_back(subtotal);
		}
	}
}

void invert(const Matrix& in, Matrix& out)
{
	// form double-wide matric in|I
	// do row-operations to convert left side to I
	// right side is inverse

	out = in;
	size_t s = in.size();

	for (size_t x = 0; x < s; x++)
	{
		out.push_back(vector<double>());
		for (size_t y = 0; y < s; y++)
		{
			out.back().push_back((x==y)?1.0f:0.0f);
		}
	}

	for (size_t z = 0; z < s; z++)
	{
		double scalar = 1.0f / out[z][z];

		for (size_t rp = 0; rp < s*2; rp++)
		{
			out[rp][z] *= scalar;
		}

		// the z-row now has 1.0 in the z-column
		// so use this to zero the z-column in every other row

		for (size_t row = 0; row < s; row++)
		{
			if (row == z)
				continue;
			// in this row, want to zero the z-column by subtracting a multiple of the z-row
			double scalar = out[z][row];
			for(size_t rp = 0; rp < s*2; rp++)
			{
				out[rp][row] -= scalar*out[rp][z];
			}
		}
	}

	Matrix both = out;
	out.clear();
	for (size_t x=0; x<s; x++)
	{
		out.push_back(vector<double>());
		for (size_t y=0; y<s; y++)
		{
			out[x].push_back(both[s+x][y]);
		}
	}

	// check
	//Matrix check; multiply(in, out, check);
}

double getRMS(Matrix& Y)
{
	assert(Y.size()==1);
	Matrix YT; transpose(Y, YT);
	Matrix YTY; multiply(YT, Y, YTY);
	return sqrt(YTY[0][0]/Y[0].size());
}

void subtract(const Matrix& left, const Matrix& right, Matrix& out)
{
	assert(left.size() == right.size());
	assert(left[0].size() == right[0].size());

	out.clear();

	for (size_t x=0; x<left.size(); x++)
	{
		out.push_back(vector<double>());
		for (size_t y=0; y<left[0].size(); y++)
		{
			out.back().push_back(left[x][y] - right[x][y]);
		}
	}
}

Matrix X; // width=p, height=N
Matrix XTXinvXT;

const int history = 3;

void generateModeller(size_t startSim)
{
	//size_t p = us.size(); // 1: just US overnight
	//size_t p = us.size() + asx.size() + 1 ; // 2: us overnight, and asx yesterday, and const
	size_t p = us.size() * history; // 3: us overnight and night before
	//size_t p = us.size() * 2 + asx.size(); // 4: us overnight and night before, and asx gapMove today
	//size_t p = us.size() * 2 + asx.size() *2; // 5: us overnight and night before, then asx yesterday, then asx gapMove today

	X.clear();

	for (size_t i=0; i<p; i++)
	{
		X.push_back(vector<double>());
	}

	for (size_t i=0; i<p; i++)
	{
		for (size_t j=0; j<startSim-1; j++)
		{
			// 1:
			//X[i].push_back(us[i][j].move);

			// 2:
			/*
			// using us and asx and 1
			if (i < us.size())
			{
				X[i].push_back(us[i][j].move);
			}
			else if (i < us.size() + asx.size())
			{
				X[i].push_back(asx[i-us.size()][j].move);
			}
			else
			{
				X[i].push_back(1.0);
			}
			*/

			
			// 3:
			// using last two days from us:
			size_t offset = i/us.size();
			if (offset > j)
				offset = j;
			X[i].push_back(us[i%us.size()][j - offset].move);
			

			// 4:
			/*
			if (i<us.size()*2)
			{
				size_t offset = 0;
				if (j>0 && i >= us.size())
					offset = 1;
				X[i].push_back(us[i%us.size()][j - offset].move);
			}
			else
			{
				Stock* thisAsx = asx[i-us.size()*2];
				float oldClose = thisAsx[j].close;
				float newOpen = thisAsx[j+1].open;
				float gapMove = 0;
				if (newOpen != 0 && oldClose != 0)
					gapMove = (newOpen/oldClose-1)*100;
				X[i].push_back(gapMove);
			}
			*/

			// 5:
			/*
			if (i<us.size()*2)
			{
				size_t offset = 0;
				if (j>0 && i >= us.size())
					offset = 1;
				X[i].push_back(us[i%us.size()][j - offset].move);
			}
			else if (i<us.size()*2+asx.size())
			{
				// yesterday's intra-day move
				Stock* thisAsx = asx[i-us.size()*2];
				X[i].push_back(thisAsx[j].move);
			}
			else
			{
				Stock* thisAsx = asx[i-us.size()*2-asx.size()];
				float oldClose = thisAsx[j].close;
				float newOpen = thisAsx[j+1].open;
				float gapMove = 0;
				if (newOpen != 0 && oldClose != 0)
					gapMove = (newOpen/oldClose-1)*100;
				X[i].push_back(gapMove);
			}
			*/
		}
	}

	cout << ".";
	Matrix XT;	transpose(X, XT);
	cout << ".";
	Matrix XTX;	multiply(XT, X, XTX);
	cout << ".";
	Matrix XTXinv;	invert(XTX, XTXinv);
	cout << ".";
	multiply(XTXinv, XT, XTXinvXT);
	cout << ".";

}

vector<double> corrs;

Matrix linearRegression(Stock* stock, size_t endData)
{
	// Stock * s has values for all dateGuide, but really only got subset of values


/*	
	vector<Stock*> indices; // each defined from 0..N-1

	int params = indices.size() + 1;

	// find the "model" for producing the real values Y from inputs X[0]..X[params-1]
	// the model B is defined as set of factors such that
	// Y = B[0] * X[0] + B[1] * X[1] + B[2] * X[2] + ... B[p-1] * X[p-1];

	// Y = X.B + e;

	// the Y vector is a vertical matrix of values in stock from 0..N-1, width=1 height=N

	// the e vector is same dimensions

	// the B vector is vertical matrix width=1, heigt=p

	// the X matrix is width=p, height=N
	//	values in first column are I[0] from 0..N-1
	//  values in second column are I[1] from 0..N-1
	// values in second-last column are last I, I[p-2]
	// values in last column are all 1.

	// SOLUTION for B is given by:

	//  B= ( (X.trns() * X).inv() * X.trns() ) * Y

	size_t start, end;
	determineStartEnd(stock, start, end);
	end++; // [ )

	// BUT need to correlate from t-1 us to t asx
	// so correlate from asx start+1 against us start ... to asx end-1 against us end-2

	size_t N = end - (start+1);
*/
	Matrix Y; // has all the stock values, N values from 0..N-1
	Matrix B; // will have p values. Num params p = numIndices+1;

	// asx values from second day of dateGuide to last day inclusive
	Y.push_back(vector<double>());
	for (size_t i=1; i<endData; i++)
	{
		float move = 0;
		if (stock[i].open != 0 && stock[i+1].open != 0)
			move = ((stock[i+1].open / stock[i].open)-1)*100;
		Y[0].push_back(move);
	}

	multiply(XTXinvXT, Y, B);

	// stock is in column matrix Y

	// y = X.B + err
	// err = y - X.B
	Matrix predicted; multiply(X, B, predicted);

	double sumsq = 0;
	for (size_t x=0; x<B[0].size(); x++)
	{
		double r = B[0][x];
		int r2 = (int)(r*10);
		double r3 = (double)r2/10;

		string name = usNames[x%usNames.size()];
		name = name.substr(1);

		cout << "," << r3;

		sumsq += (r*r);

		if ((x+1)%us.size() == 0)
		{
			cout << " (" << sumsq << ")" << endl;
			sumsq = 0;
		}
	}
	cout << endl;

	corrs.push_back(correlation(Y, predicted));
	//cout << corrs.back() << " ";

	size_t numLarge = 0; // > 2
	size_t numMed = 0; // 0.1 .. 2
	size_t numSmall = 0; // < 0.1
	for (size_t s = 0; s<B[0].size(); s++)
	{
		if (abs(B[0][s]) > 5.0f)
			numLarge++;
		else if (abs(B[0][s]) < 0.2)
			numSmall++;
		else
		{
			/*
			if (s > usNames.size()-1)
				cout << "CONST";
			else
				cout << usNames[s];
			cout << "=" << B[0][s] << " ";
			*/
			numMed++;
		}
	}
/*
	assert(numLarge == 0);
//	assert(numMed >= 1);
	assert(numSmall > B.size()/2);
*/
	// calculate RMS of stock, and RMS of errors

	/*
	double prRMS = getRMS(predicted);

	Matrix err; subtract(Y, predicted, err);

	double errRMS = getRMS(err);

	double stockRMS = getRMS(Y);

	cout << stockRMS << " " << prRMS << " " << errRMS;
	*/

	return B;
}

vector<Matrix> models;

void updateModels()
{
	size_t endData = dateToDateGuideIndex(date(2009, Mar, 9));
	cout << "Generating model matrix for dates " << dateGuide[0] << ".." << dateGuide[endData];
	generateModeller(endData);
	cout << endl;

	cout << "Step 2: Generate model parameters" << endl;

	for (size_t a = 0; a < asx.size(); a++)
	{
		cout << asxNames[a] << " =" << endl;
		size_t start, end;
		determineStartEnd(asx[a], start, end);
		models.push_back(linearRegression(asx[a], endData));
	}

	cout << endl;
}

/*
void runSimulation()
{
	size_t startSim = (size_t)(dateGuide.size() * 0.6);
	size_t endSim = (size_t)(dateGuide.size() * 0.8);

//	updateModels();

	cout << "Simulate the strategy: " << dateGuide[startSim] << ".." << dateGuide[endSim] << endl;

	double bestBalance = 10000.0f;
	double avgBalance = 10000.0f;
	double worstBalance = 10000.0f;

	cout << endl;
	cout << "Date,BestChoice,BestPred,BestAct,BestBal,WorstChoice,WorstPred,WorstAct,WorstBal" << endl;

	for (size_t day = startSim; day < endSim; day++)
	{
		if ((day-startSim)%20==0)
			updateModels();

		// models[a] is the B-matrix for asx stock a
		// a vertical column matrix
		// use this on the us values from day-1 to predict todays value

		Matrix usMoves;
		for (size_t u=0; u<us.size()+1; u++)
		{
			usMoves.push_back(vector<double>());
			if (u == us.size())
				usMoves.back().push_back(1.0f);
			else
				usMoves.back().push_back(us[u][day-1].move);
		}

		size_t bestChoice = 0;
		double bestPredictedMove;
		size_t worstChoice = 0;
		double worstPredictedMove;

		for (size_t a=0; a<asx.size(); a++)
		{
			Matrix pred;
			multiply(usMoves, models[a], pred);
			double predictedMove = pred[0][0];
			//predictedMove *= (corrs[a]*corrs[a]);

			if (bestChoice == 0 || predictedMove > bestPredictedMove)
			{
				bestChoice = a;
				bestPredictedMove = predictedMove;
			}
			if (worstChoice == 0 || predictedMove < worstPredictedMove)
			{
				worstChoice = a;
				worstPredictedMove = predictedMove;
			}
		}
		double bestActualMove = asx[bestChoice][day].move;
		double worstActualMove = asx[worstChoice][day].move;

		if (bestPredictedMove < 0.12)
			bestActualMove = 0;

		bestBalance = bestBalance * (1.0f + bestActualMove/100);
		worstBalance = worstBalance * (1.0f + worstActualMove/100);

		cout << dateGuide[day] << ", ";
		cout << asxNames[bestChoice] << ", ";
		cout << bestPredictedMove << ", ";
		cout << bestActualMove << ", ";
		cout << bestBalance << ", ";
		cout << asxNames[worstChoice] << ", ";
		cout << worstPredictedMove << ", ";
		cout << worstActualMove << ", ";
		cout << worstBalance;
		cout << endl;
	}


}
*/

void latestAdvice()
{
	date today = day_clock::local_day();
	date endData = date(2009, Mar, 9);

	size_t todayIndex = dateToDateGuideIndex(today);
	size_t endDataIndex = dateToDateGuideIndex(endData);

	cout << "Advising for investment day " << today << ":" << endl;
	cout << "Based in us moves of day " << dateGuide[todayIndex-1] << ":" << endl;

	Matrix inputs;

	//size_t p = us.size(); // 1: just US overnight
	//size_t p = us.size() + asx.size() + 1 ; // 2: us overnight, and asx yesterday, and const
	size_t p = us.size() * history; // 3: us overnight and night before
	//size_t p = us.size() * 2 + asx.size(); // 4: us overnight and night before, and asx gapMove today
	//size_t p = us.size()*2 + asx.size()*2; // 5: us overnight and night before, then asx yesterday, then asx gapMove today

	for (size_t u=0; u<p; u++)
	{
		inputs.push_back(vector<double>());

		// 1: 
		//float move = us[u][todayIndex-1].move;
		//inputs.back().push_back(move);

		// 2:
		/*
		if (u < us.size())
		{
			float move = us[u][todayIndex-1].move;
			cout << prophetToYahoo(usNames[u]) << " " << move << endl;
			inputs.back().push_back(move);
		}
		else if (u < us.size() + asx.size())
		{
			float move = asx[u-us.size()][todayIndex-1].move;
			inputs.back().push_back(move);
		}
		else
		{
			inputs.back().push_back(1.0f);
		}
		*/

		// 3:
		float move = us[u%us.size()][todayIndex-1 - u/us.size()].move;
		cout << prophetToYahoo(usNames[u%us.size()]) << " " << move << endl;
		inputs.back().push_back(move);

		// 4:
		/*
		if (u < us.size()*2)
		{
			float move = us[u%us.size()][todayIndex-1 - ((u>=us.size()) ? 1:0)].move;
			cout << prophetToYahoo(usNames[u%us.size()]) << " " << move << endl;
			inputs.back().push_back(move);
		}
		else
		{
			// TODO get today's gapMove value
			cout << asxNames[u-us.size()*2] << " TODO" << endl;
			inputs.back().push_back(0);
		}
		*/

		// 5:
		/*
		if (u < us.size()*2)
		{
			float move = us[u%us.size()][todayIndex-1 - ((u>=us.size()) ? 1:0)].move;
			cout << prophetToYahoo(usNames[u%us.size()]) << " " << move << endl;
			inputs.back().push_back(move);
		}
		else if (u < us.size()*2 + asx.size())
		{
			Stock * thisAsx = asx[u-us.size()*2];
			float move = thisAsx[todayIndex-1].move;
			cout << prophetToYahoo(asxNames[u%us.size()]) << " " << move << endl;
			inputs.back().push_back(move);
		}
		else
		{
			// TODO get today's gapMove value
			Stock * thisAsx = asx[u-us.size()*2-asx.size()];
			float newOpen = thisAsx[todayIndex].open;
			float oldClose = thisAsx[todayIndex-1].close;
			float move = 0;
			if (newOpen != 0 && oldClose != 0)
				move = (newOpen / oldClose - 1) * 100;
			cout << asxNames[u-us.size()*2 - asx.size()] << " " << move << endl;
			inputs.back().push_back(move);
		}
		*/

	}
	cout << endl;

	updateModels();

	/*
	cout << "                  ";
	for (size_t u=0; u<us.size()+1; u++)
	{
		cout << " ";
		
		if (u == us.size())
			cout << "CONST";
		else
			cout << prophetToYahoo(usNames[u]);
	}
	cout << endl;
	*/

	for (size_t a=0; a<asx.size(); a++)
	{
		// asx values from second day of dateGuide to last day inclusive
		Matrix Y;
		Y.push_back(vector<double>());
		size_t start, end;
		determineStartEnd(asx[a], start, end);
		for (size_t i=start+1; i<endDataIndex; i++)
		{
			Y[0].push_back(asx[a][i].move);
		}

		Matrix predicted; multiply(X, models[a], predicted);
		double r = correlation(Y, predicted);

		Matrix pred;
		multiply(inputs, models[a], pred);
		double s = pred[0][0]; // predictedMove
		double e = s * r * r; // part of predictedMove that is explained by correlation
		cout << asxNames[a];
		cout << "  \te=" << e;
		cout << "  \ts=" << s;
		cout << "  \tr=" << r;

		/*
		for (size_t u=0; u<us.size()+1; u++)
		{
			cout << " " << models[a][0][u];
		}
		*/
		cout << endl;
	}

}

Stock* findStock(const string& symbol, date d)
{
	Stock * result = 0;

	size_t n=0;
	for (; n<asxNames.size(); n++)
		if (asxNames[n] == symbol)
			break;

	if (n == asxNames.size())
		return result;

	Stock* thisAsx = asx[n];

	size_t s = dateToDateGuideIndex(d);

	result = &thisAsx[s];

	return result;
}

int main()
{

	// using fmtflags as a type:
    ios_base::fmtflags ff;
    ff = cout.flags();
    //ff &= ~cout.basefield;   // unset basefield bits
    ff |= cout.showpos;          // set hex
    cout.flags(ff);
	  
	cout.precision(3);
	cout.width(8);
	cout.fill('x');
	cout.right;


	//;	cout.setf(ios::fixed,ios::floatfield|ios::showpos);

	// go through each stock, and determine:
	//	start date, end date, number of days, average volume, average absolute percentage move

	// don't just pick large files, because
	// eg: amp file size is 51KB because runs from 1998 to 2002
	
	// approach will be to use cd data as guide for what to download,
	// then to download all up to present Oct 2007
	// then test sim from start to finish

	generateDateGuide();

	//loadTextSaveData("data/asx");
	//loadTextSaveData("data/us");

	//loadTextWeblinkSaveData("data/asx");

	//summarizeDirectory("data/asx");
	//summarizeDirectory("data/us");

	us = loadDataStocks("data/us");
	asx = loadDataStocks("data/asx");
/*
	Stock* s = 0;

	s = findStock("XDJ", date(2009, Mar, 24)); s->open = 1098; s->close = 1091;	setMove(s);
	s = findStock("XDJ", date(2009, Mar, 25)); s->open = 1092; s->close = 1105; setMove(s);

	s = findStock("XEJ", date(2009, Mar, 24)); s->open = 13350; s->close = 13250; setMove(s);
	s = findStock("XEJ", date(2009, Mar, 25)); s->open = 13480; s->close = 13560; setMove(s);

	s = findStock("XFJ", date(2009, Mar, 24)); s->open = 3470; s->close = 3365; setMove(s);
	s = findStock("XFJ", date(2009, Mar, 25)); s->open = 3395; s->close = 3448; setMove(s);

	s = findStock("XHJ", date(2009, Mar, 24)); s->open = 7480; s->close = 7390; setMove(s);
	s = findStock("XHJ", date(2009, Mar, 25)); s->open = 7500; s->close = 7640; setMove(s);

	s = findStock("XIJ", date(2009, Mar, 24)); s->open = 507; s->close = 508; setMove(s);
	s = findStock("XIJ", date(2009, Mar, 25)); s->open = 494; s->close = 494; setMove(s);

	s = findStock("XMJ", date(2009, Mar, 24)); s->open = 9450; s->close = 9360; setMove(s);
	s = findStock("XMJ", date(2009, Mar, 25)); s->open = 9230; s->close = 9260; setMove(s);

	s = findStock("XNJ", date(2009, Mar, 24)); s->open = 2670; s->close = 2628;	setMove(s);
	s = findStock("XNJ", date(2009, Mar, 25)); s->open = 2605; s->close = 2588; setMove(s);

	s = findStock("XSJ", date(2009, Mar, 24)); s->open = 5950; s->close = 5950;	setMove(s);
	s = findStock("XSJ", date(2009, Mar, 25)); s->open = 5980; s->close = 5995; setMove(s);

	s = findStock("XTJ", date(2009, Mar, 24)); s->open = 1091; s->close = 1079;	setMove(s);
	s = findStock("XTJ", date(2009, Mar, 25)); s->open = 1082; s->close = 1079; setMove(s);

	s = findStock("XUJ", date(2009, Mar, 24)); s->open = 3935; s->close = 3889;	setMove(s);
	s = findStock("XUJ", date(2009, Mar, 25)); s->open = 3940; s->close = 3961; setMove(s);
*/

	/*
	// randomize the asx data, to check model correlation - expect lower r values
	srand(time(0));
	for (size_t a=0; a<asx.size(); a++)
	{
		for (size_t d=0; d<dateGuide.size(); d++)
		{
			asx[a][d].move = ((float)rand()/RAND_MAX-0.5)*5;
		}
	}
	*/

	// show unusual data points, as sanity check
	for (size_t a=0; a<asx.size(); a++)
	{
		for (size_t d=0; d<dateGuide.size(); d++)
		{
			if (fabs(asx[a][d].move) > 10)
			{
				cout << asxNames[a];
				cout << " ";
				cout << dateGuide[d];
				cout << " ";
				cout << asx[a][d].open;
				cout << " ";
				cout << asx[a][d].close;
				cout << " ";
				cout << asx[a][d].move << endl;
			}
		}
	}


//	runSimulation();

	/*

	for (size_t a=0; a<asx.size(); a++)
	{
		//if (asxNames[a][0] < 'S')
		//	continue;

		cout << asxNames[a] << ": ";

		linearRegression(asx[a]);

		cout << endl;
	}


*/

//	correlationGrid();

	latestAdvice();

	return 0;
}

