 
/** 
 *  @file    Var_w.cpp
 *  @author  Sukriti Gupta
 *  @date    19/7/2018  
 *  @version 1.0 
 *  
 *  @brief Front end for pragma handling - Elaborate step
 *
 *  @section DESCRIPTION
 *  
 *  This program parses through an input file two times.
 *  The first pass is used to record all the various datatypes of variable pragmas.
 *  The second pass is used to create the output file with the necessary file information.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <set>
#include <map>

#define INIT_STRING	"\n\r\n\r\
******************************************************************\n\r\
******Dynamic High-Level Synthesis Compiler **********************\n\r\
******Andrea Guerrieri - Lana Josipovic - EPFL-LAP 2019 **********\n\r\
******Source Code Elaboration Tool********************************\n\r\
******************************************************************\n\r\
";

using namespace std;

string line;///< Variable used to store current line being read
string subs, subs1, subs2;///< Variables used to store current words being read
ifstream infile;///< File handle for input file - should not have comments
//("in.cpp");
//ofstream tempfile ("t.cpp");
ofstream outfile;///< File handle for elaborated output file
// ("out.cpp");

//std::set<std::string> datatypes;
std::set<std::string> addt;
map <string, std::set<std::string> > datatyp;  ///< Map used to store the various datatypes with the set of arrays or simple usage being used in the variable pragmas (so that necessary functions can be created)      


/*! A recursive function to check for empty lines and print them to the output. Returns whenever it finds the first non - empty line. */
void check_new_line()
{
	if(line.length()==0||line.find_first_not_of(' ') == std::string::npos)
	{
		outfile<<endl;
		if(getline (infile,line))
			check_new_line();
		
	}
}

/*! A recursive function to check for empty lines and does not print anything to the output. Returns whenever it finds the first non - empty line. */
void check_new_linewp()
{
	if(line.length()==0||line.find_first_not_of(' ') == std::string::npos)
	{
		if(getline (infile,line))
			check_new_linewp();
		
	}
}


int main (int argc, char **argv) {

    // infile.open(argv[1]);
  //   std::string source;

  //   getline(infile, source, '\0');
  //   while(source.find("/*") != std::string::npos) {
  //     size_t Beg = source.find("/*");
  //     source.erase(Beg, (source.find("*/", Beg) - Beg)+2);
  //   }
  //   while(source.find("//") != std::string::npos) {
  //     size_t Beg = source.find("//");
  //     source.erase(Beg, source.find("\n", Beg) - Beg);
  //   }
  // tempfile << source;
  // infile.close();
  // tempfile.close();

  /*! Opening the correct files */
  
    cout << INIT_STRING;

  infile.open(argv[1]);
  outfile.open(argv[2]);

  int pragma=0;//keeps count of the number of non var pragmas ///////////////+++++++
  

  /*! First pass to record datatypes for var pragmas*/
  if (infile.is_open())
  {
    while ( getline (infile,line) )
    {    	
    	check_new_linewp();
		istringstream iss(line);
        iss >> subs;

        if(subs.compare("#pragma")==0)
        {
        	iss >> subs;
        	if(subs.compare("dhls")==0)
        	{
        		iss>>subs;
        		if(subs.compare("var")==0)
        		{
        			iss>>subs;
              if(subs.compare("add")==0)
              {
                iss>>subs1; //what to add
                iss>>subs2; //datatype of what to add

                if(datatyp.find(subs2)!=datatyp.end())
                {
                  addt=datatyp.find(subs2)->second;
                  addt.insert(subs1);
                  datatyp.erase(datatyp.begin(), datatyp.find(subs2));
                  datatyp.insert(pair <string,  std::set<std::string> >(subs2, addt));
                }
                else
                {
                  std::set<std::string> addu;
                  addu.insert(subs1);
                  datatyp.insert(pair <string,  std::set<std::string> >(subs2, addu));
                }

                //iss>>subs;
              }
              else
              {
                iss>>subs2;//name of variable
                if(datatyp.find(subs)!=datatyp.end())
                {
                  addt=datatyp.find(subs)->second;
                  addt.insert(" ");
                  datatyp.erase(datatyp.begin(), datatyp.find(subs));
                  datatyp.insert(pair <string,  std::set<std::string> >(subs, addt));
                }
                else
                {
                  std::set<std::string> addu;
                  addu.insert(" ");
                  datatyp.insert(pair <string,  std::set<std::string> >(subs, addu));
                }


        			  //datatypes.insert(subs);
              }
        		}

            else pragma++; ///////////////+++++++
        	}
        }                    
    }
    infile.close();    
  }
  else cout << "Unable to open file"; 


  /*! Second pass to create the output file with the necessary information*/
  infile.open (argv[1]);
  if (infile.is_open() && outfile.is_open())
  {

    if(pragma!=0)///////////////+++++++
    {
      outfile<<"extern void __my_dhls_label(char* label) {}"<<endl;  ///< Printing the global function for the calls for generic, loop and function pragmas
    }
    
    map <string, std::set<std::string> > :: iterator itr;
    for (itr = datatyp.begin(); itr != datatyp.end(); ++itr)
    {
      for (set<string>::iterator it=itr->second.begin(); it!=itr->second.end(); ++it)
        outfile<<"extern void __my_dhls_label_with_var("<<(itr->first)<<" var"<<*it<<", char* label) {}"<<endl;  ///< Printing the global function for the calls for variable pragmas for each kind of array that might be used
    }

    while ( getline (infile,line) )
    {
    	check_new_line();
		  istringstream iss(line);
      iss >> subs;

        if(subs.compare("#pragma")==0)
        {
        	iss >> subs;
        	if(subs.compare("dhls")==0)
        	{
        		iss>>subs;
        		if(subs.compare("var")==0)
        		{

              iss>>subs;

              if(subs.compare("add")==0)
              {
                iss>>subs;
                iss>>subs;
                iss>>subs;
                outfile<<"__my_dhls_label_with_var("<< subs<<", \" "<<line<<"\");"<<endl; ///PLacing the right function call

              }
              else
              {
                iss>>subs;
                outfile<<"__my_dhls_label_with_var("<< subs<<", \" "<<line<<"\");"<<endl; ///PLacing the right function call
              }
        		}
            else if(subs.compare("func")==0 || subs.compare("loop")==0 || subs.compare("operation")==0 || subs.compare("generic")==0 )
            {
                outfile<<"__my_dhls_label(\""<< line<<"\");"<<endl;    ///PLacing the right function call             
            }
        		else
        		outfile << line<<endl;
        	}
        	else
        		outfile << line<<endl;
        }
        else
        {        	
        	outfile << line<<endl;        	
        }             
    }
    infile.close();
    outfile.close();
  }
  else cout << "Unable to open file"; 

  return 0;
}

