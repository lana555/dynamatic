#include "ElasticPass/Pragmas.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// using namespace std;

using namespace std;

//  struct PragmaDetails {
//  	int type;///< 0 for func, 1 for var, 2 for loop
//  	string name;///<Name of variable or function as it appears in the IR, "LOOP" in case of loop
//  	string info;///< The information that was needed to be passed
// };

vector<struct PragmaDetails> all_pragma;

ifstream infile;
map<string, string> notation_map;
string str, line, subs;
ofstream finfile;

int i = 0;

void check_new_linewp() {
    if (line.length() == 0 || line.find_first_not_of(' ') == std::string::npos) {
        if (getline(infile, line)) {
            ++i;
            check_new_linewp();
        }
    }
}

void check_new_line() {
    if (line.length() == 0 || line.find_first_not_of(' ') == std::string::npos) {
        finfile << endl;
        if (getline(infile, line))
            check_new_line();
    }
}

void pragmas(int rem, string filename) {
    static int count = 0;
    cout << count++;

    if (count == 1) {
        infile.open(filename);
        std::string source;
        if (infile.is_open()) {
            while (getline(infile, line)) {
                ++i;
                check_new_linewp();
                istringstream iss(line);
                iss >> subs;

                if (subs.find("@.str") != std::string::npos) {
                    str =
                        line.substr(line.find("\"") + 1, (line.rfind("\"") - line.find("\"") - 4));
                    notation_map.insert(pair<string, string>(subs, str));
                } else {
                    if (line.find("__my_dhls_label_with_var") != std::string::npos &&
                        line.find("define") == std::string::npos) {
                        struct PragmaDetails cpragma;
                        cpragma.type = 1;
                        std::istringstream ss(line);
                        std::string token;
                        string temp, word;
                        while (ss >> temp) {
                            if (temp.substr(temp.length() - 1, temp.length()) == ",")
                                temp = temp.substr(0, temp.length() - 1);
                            if (temp.find("@.str") != std::string::npos) {
                                cpragma.info = notation_map.find(temp)->second;
                                break;
                            }
                        }
                        istringstream viss(cpragma.info);
                        viss >> word;
                        viss >> word;
                        viss >> word;
                        viss >> word;
                        if (word.compare("add") == 0) {
                            viss >> word;
                            viss >> word;
                        }
                        viss >> word;
                        cpragma.name = word;
                        all_pragma.push_back(cpragma);
                    }

                    else if (line.find("__my_dhls_label") != std::string::npos &&
                             line.find("define") == std::string::npos) {
                        struct PragmaDetails cpragma;
                        std::istringstream ss(line);
                        std::string token;
                        string temp, word;

                        while (ss >> temp) {
                            if (temp.substr(temp.length() - 1, temp.length()) == ",")
                                temp = temp.substr(0, temp.length() - 1);
                            if (temp.find("@.str") != std::string::npos) {
                                cpragma.info = notation_map.find(temp)->second;
                                break;
                            }
                        }
                        istringstream viss(cpragma.info);
                        viss >> word;
                        viss >> word;
                        viss >> word;

                        if (word.compare("loop") == 0) {
                            cpragma.type = 2;
                            viss >> word;
                            cpragma.name = word;
                        } else if (word.compare("func") == 0) {
                            cpragma.type = 0;
                            viss >> word;
                            cpragma.name = word;
                        } else if (word.compare("operation") == 0) {
                            cpragma.type = 3;
                            viss >> word; // operator
                            viss >> word; // name of line
                            cpragma.name = word;
                        } else if (word.compare("generic") == 0) {
                            cpragma.type = 4;
                            string inf   = "";

                            while (viss >> word) {
                                inf = inf + " " + word;
                            }
                            cpragma.name = inf;
                        }

                        all_pragma.push_back(cpragma);
                    }
                }
            }
            infile.close();
        } else
            cout << "Unable to open file";
        vector<struct PragmaDetails>::iterator i;

        ofstream outfile("out.txt");
        for (i = all_pragma.begin(); i != all_pragma.end(); ++i) {
            cout << (*i).type << '\t' << (*i).name << (*i).info << endl;
            outfile << (*i).type << '\t' << (*i).name << (*i).info << endl;
        }
        outfile.close();

        if (rem == 1) {
            infile.open(filename);
            finfile.open("tempoutput.ll");

            if (infile.is_open()) {
                while (getline(infile, line)) {
                    ++i;
                    check_new_line();
                    if ((line.find("__my_dhls_label_with_var") != std::string::npos ||
                         line.find("__my_dhls_label") != std::string::npos) &&
                        line.find("define") == std::string::npos &&
                        line.find("call") != std::string::npos) {
                        continue;
                    } else
                        finfile << line << endl;
                }
                infile.close();
                finfile.close();
                string newf = "original_" + filename;
                rename(filename.c_str(), newf.c_str());
                rename("tempoutput.ll", filename.c_str());
            } else
                cout << "Unable to open file";
        }
    }
}

string get_pragma_loop(string loop_name) {
    string word;
    vector<struct PragmaDetails>::iterator i;
    for (i = all_pragma.begin(); i != all_pragma.end(); ++i) {
        if ((*i).type == 2 && loop_name.compare((*i).name) == 0) {
            istringstream iss((*i).info);
            iss >> word; //#pragma
            iss >> word; // dhls
            iss >> word; // loop
            iss >> word; // name

            string inf = "";

            while (iss >> word) {
                inf = inf + " " + word;
            }
            return inf;
        }
    }

    string s = "No Pragma Found";
    return s;
}

bool get_pragma_generic(string gen) {
    bool ret;
    string infword, word;
    vector<struct PragmaDetails>::iterator i;
    for (i = all_pragma.begin(); i != all_pragma.end(); ++i) {
        if ((*i).type == 4) {
            istringstream iss(gen);
            istringstream ss((*i).name);
            ret = true;
            while (ss >> infword) {
                if (iss >> word) {
                    if (word != infword) {
                        ret = false;
                        break;
                    }
                } else {
                    ret = false;
                    break;
                }
            }
            if (ret) {
                istringstream isss(gen);
                istringstream sss((*i).name);
                while (isss >> word) {
                    if (sss >> infword) {
                        if (word != infword) {
                            ret = false;
                            break;
                        }
                    } else {
                        ret = false;
                        break;
                    }
                }
            }
            if (ret)
                return ret;
        }
    }
    return false;
}

string get_pragma_func(string func_name) {
    vector<struct PragmaDetails>::iterator i;
    for (i = all_pragma.begin(); i != all_pragma.end(); ++i) {
        if ((*i).type == 0 && func_name.compare((*i).name) == 0) {
            string word;
            istringstream iss((*i).info);
            iss >> word; //#pragma
            iss >> word; // dhls
            iss >> word; // func
            iss >> word; // name

            string inf = "";

            while (iss >> word) {
                inf = inf + " " + word;
            }
            return inf;
        }
    }

    string s = "No Pragma Found";
    return s;
}

string get_pragma_op(string line_label, char op) {
    vector<struct PragmaDetails>::iterator i;
    for (i = all_pragma.begin(); i != all_pragma.end(); ++i) {
        if ((*i).type == 3 && line_label.compare((*i).name) == 0) {
            string word;
            istringstream iss((*i).info);
            iss >> word; //#pragma
            iss >> word; // dhls
            iss >> word; // operation
            iss >> word; // operator

            stringstream ss;
            string str;
            ss << op;
            ss >> str;

            if (str.compare(word) == 0) {
                iss >> word; // name of line
                string inf = "";

                while (iss >> word) {
                    inf = inf + " " + word;
                }
                return inf;
            }
        }
    }

    string s = "No Pragma Found";
    return s;
}

string get_pragma_var(string data_type, string var, string arr) {
    vector<struct PragmaDetails>::iterator i;
    for (i = all_pragma.begin(); i != all_pragma.end(); ++i) {
        if ((*i).type == 1 && var.compare((*i).name) == 0) {
            bool temp = true;
            // string inpdtext; //input datatype extension
            string word;
            istringstream iss((*i).info);
            iss >> word; //#pragma
            iss >> word; // dhls
            iss >> word; // var
            iss >> word;

            if (word.compare("add") == 0) {
                iss >> word;
                temp = (word == arr);
                iss >> word;
            }

            if (temp) {
                if (word.compare(data_type) == 0) {

                    iss >> word; // name of line
                    string inf = "";

                    while (iss >> word) {
                        inf = inf + " " + word;
                    }
                    return inf;
                }
            }
        }
    }

    string s = "No Pragma Found";
    return s;
}

// int main()
// {
// 	test();
// 	return 0;
// }

// std::vector<struct PragmaDetails> all_pragma;
//
// std::ifstream infile;
// std::map <std::string, std::string> notation_map;
// std::string str, line,subs;
// std::ofstream finfile;
//
// int i=0;
//
// void check_new_linewp()
// {
//     if(line.length()==0||line.find_first_not_of(' ') == std::string::npos)
//     {
//         if(getline (infile,line))
//         {
//             ++i;
//             check_new_linewp();
//         }
//     }
// }
//
// void check_new_line()
// {
//     if(line.length()==0||line.find_first_not_of(' ') == std::string::npos)
//     {
//         finfile<<std::endl;
//         if(getline (infile,line))
//             check_new_line();
//
//     }
// }
//
//
// void pragmas ( int rem, std::string filename )
// {
//     static int count=0;
//     std::cout << count++;
//
//     struct PragmaDetails cpragma;
//
//     if ( count == 1 )
//     {
//         infile.open(filename);
//         std::string source;
//         if (infile.is_open())
//         {
//             while ( getline (infile,line) )
//             {
//                 ++i;
//
//                 if( ( line.length()==0 || line.find_first_not_of(' ') == std::string::npos ) )
//                 {
// 		  continue;
// 		}
//                     std::istringstream iss(line);
//                     iss >> subs;
//
//                     if (subs.find("@.str") != std::string::npos)
//                     {
// 		      	str=line.substr(  line.find("\"") + 1,  (line.rfind("\"") - line.find("\"")
// )  );
//                         notation_map.insert(std::pair <std::string, std::string >(subs, str));
//                     }
//                     else
//                     {
//                         if(line.find("__my_dhls_label_with_var")!=std::string::npos &&
//                         line.find("define")==std::string::npos)
//                         {
//                             //struct PragmaDetails cpragma;
//                             cpragma.type= 1;
//                             std::istringstream ss(line);
//                             std::string token;
//                             std::string temp, word;
//                             while(ss>>temp)
//                             {
//                                 if(temp.substr(  temp.length() -1, temp.length() )=="," )
//                                     temp= temp.substr( 0, temp.length() -1 );
//                                 if(temp.find("@.str")!= std::string::npos)
//                                 {
//                                     cpragma.info=notation_map.find(temp)->second;
//                                     break;
//                                 }
//                             }
//                             std::istringstream viss(cpragma.info);
//                             viss>>word;
//                             viss>>word;
//                             viss>>word;
//                             viss>>word;
//                             if(word.compare("add")==0)
//                             {
//                                 viss>>word;
//                                 viss>>word;
//                             }
//                             viss>>word;
//                             cpragma.name=word;
//                             all_pragma.push_back(cpragma);
//                         }
//
//                         else if(line.find("__my_dhls_label")!=std::string::npos   &&
//                         line.find("define")==std::string::npos)
//                         {
//                             std::istringstream ss(line);
//                             std::string token;
//                             std::string temp, word;
//
//                             while(ss>>temp)
//                             {
//                                 if(temp.substr(  temp.length() -1, temp.length() )=="," )
//                                     temp= temp.substr( 0, temp.length() -1 );
//                                 if(temp.find("@.str")!= std::string::npos)
//                                 {
//                                     cpragma.info=notation_map.find(temp)->second;
//                                     break;
//                                 }
//                             }
//                             std::istringstream viss(cpragma.info);
//                             viss>>word;
//                             viss>>word;
//                             viss>>word;
//
//                             if(word.compare("loop")==0)
//                             {
//                                 cpragma.type= 2;
//                                 viss>>word;
//                                 cpragma.name=word;
//                             }
//                             else if(word.compare("func")==0)
//                             {
//                                 cpragma.type= 0;
//                                 viss>>word;
//                                 cpragma.name=word;
//                             }
//                             else if(word.compare("operation")==0)
//                             {
//                                 cpragma.type= 3;
//                                 viss>>word; //operator
//                                 viss>>word; //name of line
//                                 cpragma.name=word;
//                             }
//                             else if(word.compare("generic")==0)
//                             {
//                                 cpragma.type= 4;
//                                 std::string inf="";
//
//                                 while(viss>>word)
//                                 {
//                                     inf=inf+" "+subs;
//                                 }
//                                 cpragma.name=inf;
//                             }
//
//                             all_pragma.push_back(cpragma);
//                         }
//                     }
//
//             }
//             infile.close();
//         }
//         else std::cout << "Unable to open file";
//         std::vector <struct PragmaDetails> :: iterator i;
//
//         std::ofstream outfile("out.txt");
//         for (i = all_pragma.begin(); i != all_pragma.end(); ++i)
//         {
//             std::cout << (*i).type<< '\t' << (*i).name<< (*i ).info<<std::endl;
//             outfile << (*i).type<< '\t' << (*i).name<< (*i ).info<<std::endl;
//         }
//         outfile.close();
//
//         if(rem==1)
//         {
//             infile.open(filename);
//             finfile.open("tempoutput.ll");
//
//             if (infile.is_open())
//             {
//                 while ( getline (infile,line) )
//                 {
//                     ++i;
//                     check_new_line();
//                     if((line.find("__my_dhls_label_with_var")!=std::string::npos ||
//                     line.find("__my_dhls_label")!=std::string::npos) &&
//                     line.find("define")==std::string::npos  &&
//                     line.find("call")!=std::string::npos)
//                     {
//                         continue;
//                     }
//                     else
//                         finfile<<line<<std::endl;
//                 }
//                 infile.close();
//                 finfile.close();
//                 std::string newf="original_"+filename;
//                 rename(filename.c_str(),newf.c_str());
//                 rename("tempoutput.ll",filename.c_str());
//             }
//             else std::cout << "Unable to open file";
//         }
//     }
// }
//
//
// std::string get_pragma_loop(std::string loop_name)
// {
//     std::string word;
//     std::vector <struct PragmaDetails> :: iterator i;
//     for (i = all_pragma.begin(); i != all_pragma.end(); ++i)
//     {
//         if((*i).type==2 && loop_name.compare((*i).name)==0)
//         {
//             std::istringstream iss((*i ).info);
//             iss>>word; //#pragma
//             iss>>word; //dhls
//             iss>>word; //loop
//             iss>>word; //name
//
//             std::string inf="";
//
//             while(iss>>word)
//             {
//                 inf=inf+" "+word;
//             }
//             return inf;
//         }
//     }
//
//     std::string s="No Pragma Found";
//     return s;
// }
//
// bool get_pragma_generic(std::string gen)
// {
//     bool ret;
//     std::string infword, word;
//     std::vector <struct PragmaDetails> :: iterator i;
//     for (i = all_pragma.begin(); i != all_pragma.end(); ++i)
//     {
//         if((*i).type==4)
//         {
//             std::istringstream iss(gen);
//             std::istringstream ss((*i).name);
//             ret=true;
//             while(ss>>infword)
//             {
//                 if(iss>>word)
//                 {
//                     if(word!=infword)
//                     {
//                         ret=false;
//                         break;
//                     }
//                 }
//                 else
//                 {
//                     ret=false;
//                     break;
//                 }
//             }
//             if(ret)
//             {
//                 std::istringstream isss(gen);
//                 std::istringstream sss((*i).name);
//                 while(isss>>word)
//                 {
//                     if(sss>>infword)
//                     {
//                         if(word!=infword)
//                         {
//                             ret=false;
//                             break;
//                         }
//                     }
//                     else
//                     {
//                         ret=false;
//                         break;
//                     }
//
//                 }
//             }
//             if(ret)
//                 return ret;
//         }
//     }
//     return false;
// }
//
// std::string get_pragma_func(std::string func_name)
// {
//     std::vector <struct PragmaDetails> :: iterator i;
//     for (i = all_pragma.begin(); i != all_pragma.end(); ++i)
//     {
//         if((*i).type==0 && func_name.compare((*i).name)==0)
//         {
//             std::string word;
//             std::istringstream iss((*i ).info);
//             iss>>word; //#pragma
//             iss>>word; //dhls
//             iss>>word; //func
//             iss>>word; //name
//
//             std::string inf="";
//
//             while(iss>>word)
//             {
//                 inf=inf+" "+word;
//             }
//             return inf;
//         }
//     }
//
//     std::string s="No Pragma Found";
//     return s;
// }
//
// std::string get_pragma_op(std::string line_label, char op)
// {
//     std::vector <struct PragmaDetails> :: iterator i;
//     for (i = all_pragma.begin(); i != all_pragma.end(); ++i)
//     {
//         if((*i).type==3 && line_label.compare((*i).name)==0)
//         {
//             std::string word;
//             std::istringstream iss((*i ).info);
//             iss>>word; //#pragma
//             iss>>word; //dhls
//             iss>>word; //operation
//             iss>>word; //operator
//
//             std::stringstream ss;
//             std::string str;
//             ss << op;
//             ss >> str;
//
//             if(str.compare(word)==0)
//             {
//                 iss>>word; //name of line
//                 std::string inf="";
//
//                 while(iss>>word)
//                 {
//                     inf=inf+" "+word;
//                 }
//                 return inf;
//             }
//         }
//     }
//
//     std::string s="No Pragma Found";
//     return s;
// }
//
//
// std::string get_pragma_var(std::string data_type, std::string var, std::string arr)
// {
//     std::vector <struct PragmaDetails> :: iterator i;
//     for (i = all_pragma.begin(); i != all_pragma.end(); ++i)
//     {
//         if((*i).type==1 && var.compare((*i).name)==0)
//         {
//             bool temp=true;
//             //string inpdtext; //input datatype extension
//             std::string word;
//             std::istringstream iss((*i ).info);
//             iss>>word; //#pragma
//             iss>>word; //dhls
//             iss>>word; //var
//             iss>>word;
//
//             if(word.compare("add")==0)
//             {
//                 iss>>word;
//                 temp=(word==arr);
//                 iss>>word;
//             }
//
//             if(temp)
//             {
//                 if(word.compare(data_type)==0)
//                 {
//
//                     iss>>word; //name of line
//                     std::string inf="";
//
//                     while(iss>>word)
//                     {
//                         inf=inf+" "+word;
//                     }
//                     return inf;
//
//                 }
//             }
//
//         }
//     }
//
//     std::string s="No Pragma Found";
//     return s;
// }
