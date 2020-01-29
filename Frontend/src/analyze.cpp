/*
*  C++ Implementation: dhls
*
* Description:
*
*
* Author: Andrea Guerrieri <andrea.guerrieri@epfl.ch (C) 2019
*
* Copyright: See COPYING file that comes with this distribution
*
*/



#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#define INIT_STRING	"\n\r\n\r\
******************************************************************\n\r\
******Dynamic High-Level Synthesis Compiler **********************\n\r\
******Andrea Guerrieri - Lana Josipovic - EPFL-LAP 2019 **********\n\r\
******Source Code Analyzer Tool***********************************\n\r\
******************************************************************\n\r\
";

using namespace std;

string line1, line2, line3, subs1, subs2, subs, tsubs;
string file_path, final_file;
ifstream infile, file;
ofstream outfile;
int i1=1, i2=1, i3=1;



std::string trim(const std::string& str,
                 const std::string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}


void check_new_linewp1()
{
    if(line1.length()==0||line1.find_first_not_of(' ') == std::string::npos)
    {
        i1++;
        if(getline (infile,line1))
            check_new_linewp1();        
    }
}
void check_new_linewp2()
{
    if(line2.length()==0||line2.find_first_not_of(' ') == std::string::npos)
    {
        i2++;
        if(getline (infile,line2))
            check_new_linewp2();        
    }
}

void check_new_linewp3()
{
    if(line3.length()==0||line3.find_first_not_of(' ') == std::string::npos)
    {
        i3++;
        if(getline (infile,line3))
            check_new_linewp3();        
    }
}


bool loop_finder(string line)
{
    if(line.find("for") != std::string::npos|| line.find("while") != std::string::npos ||line.find("do") != std::string::npos)
    {
        istringstream iss(line);
        while(iss >> subs) //to safeguard against function which might have have the keywords as a part of their name
        {
            if(subs=="for"||subs=="do"||subs=="while")
            {
                return true;
            }
        }
    }
    return false;
}

/*! A Function to check whether or not a specific pragma has been implemented or not from a list of pragmas */
bool pragma_present( string inf, string filename )
{
    final_file=file_path+filename;
    file.open(final_file);
    bool ret=true;
    string word, infword, line;
    int i=0;

    if(file.is_open())
    { 
        while(getline (file,line))
        {
            ++i;
            istringstream iss(line);
            istringstream ss(inf);
            ret=true;
            while(ss>>infword)
            {
                if(iss>>word)
                {
                    if(word!=infword)
                    {
                        ret=false;
                        break;
                    }
                }
                else 
                {
                    ret=false;
                    break;
                }
            }
            if(ret)
            {
                istringstream isss(line);
                istringstream sss(inf);
                while(isss>>word)
                {
                    if(sss>>infword)
                    {
                        if(word!=infword)
                        {
                            ret=false;
                            break;
                        }
                    }
                    else 
                    {
                        ret=false;
                        break;
                    }

                }
            }
            if(ret)
            {
                file.close();
                return ret;
            }
        }
        file.close();

        if(i==0)
            return false;
        return ret;
    }
    else
        cout<<"Error in opening "<<filename<<endl;
}

int main (int argc, char **argv) {


    infile.open(argv[1]);
    outfile.open(argv[2]);
    std::string source;
    file_path=argv[3];

    cout << INIT_STRING;

    ///Removing comments

    getline(infile, source, '\0');
    while(source.find("/*") != std::string::npos) {
        size_t Beg = source.find("/*");
        source.erase(Beg, (source.find("*/", Beg) - Beg)+2);
    }
    while(source.find("//") != std::string::npos) {
        size_t Beg = source.find("//");
        source.erase(Beg, source.find("\n", Beg) - Beg);
    }
        
    
    outfile << source;
    
    infile.close();
    outfile.close();

    infile.open(argv[2]);
    if(infile.is_open())
    {
        //getline (infile,line1);
        //check_new_linewp1;
        
        getline (infile,line2);
        i2=1;
        check_new_linewp2;
        getline (infile,line3);
        i3=2;
        

        do
        {
            check_new_linewp3;

            //////////Generic pragma checking in file
            istringstream tiss2(line2);
            tiss2 >> subs;
            if(subs.compare("#pragma")==0)
            {
                tiss2 >> subs;
                if(subs.compare("dhls")==0)
                {
                    tiss2>>subs;
                    if(subs.compare("generic")==0)
                    {                        
                        string inf="";

                        while(tiss2>>subs)
                        {
                            inf=inf+" "+subs;
                        }

                        if(!pragma_present(inf,"generic.txt"))
                        {
                            cout<<argv[2]<<": Line "<<i2<<" Warning: Pragma \""<<inf<<"\" not implemented from  \""<<trim(line2)<<"\""<<endl;////////////////////////////////////////////////////////////////////////////////////////
                        }

                    }
                }
            }


            //////////Operation pragma checking in next line
            istringstream iss2(line2);
            iss2 >> subs;
            if(subs.compare("#pragma")==0)
            {
                iss2 >> subs;
                if(subs.compare("dhls")==0)
                {
                    iss2>>subs;
                    if(subs.compare("operation")==0)
                    {
                        iss2>>subs1; //operator
                        iss2>>subs2; //name

                        if(line3.find(subs1)==string::npos)
                        {
                            cout<<argv[2]<<": Line "<<i3<<" Error: Missing operator \""<<subs1<<"\" in \""<<trim(line3)<<"\" which is required by \""<<trim(line2)<<"\""<<endl;////////////////////////////////////////////////////
                        }

                        if(line3.find(subs2)==string::npos)
                        {
                            cout<<argv[2]<<": Line "<<i2<<" Error: \""<<subs2<<"\" No such name exists as required by \""<<trim(line2)<<"\""<<endl;///////////////////////////////////////////////////
                        }

                        //else
                        //{
                            string inf="";

                            while(iss2>>subs)
                            {
                                inf=inf+" "+subs;
                            }

                            if(!pragma_present(inf,"operation.txt"))
                            {
                                cout<<argv[2]<<": Line "<<i2<<" Warning: Pragma \""<<inf<<"\" not implemented from  \""<<trim(line2)<<"\""<<endl;////////////////////////////////////////////////////////////////////////////////////////
                            }
                        //}

                    }
                }
            }

            //////////Var pragma checking in prev line
            istringstream iss(line3);
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
                        }

                        iss>>subs1; //name of var, datatype in subs

                        if(line2.find(subs1)==string::npos||line2.find(subs)==string::npos)
                        {
                            cout<<argv[2]<<": Warning: Line "<<i2<<" Variable \""<<subs1<<"\" of type \""<<subs<<"\" has not been declared from \""<<trim(line3)<<"\""<<endl; ///////////////////////////////////////////////////////////////
                        }
                        //else
                        //{
                            string inf="";

                            while(iss>>subs)
                            {
                                inf=inf+" "+subs;
                            }

                            if(!pragma_present(inf,"var.txt"))
                            {
                                cout<<argv[2]<<": Line "<<i3<<" Warning: Pragma \""<<inf<<"\" not implemented as required by \""<<trim(line3)<<"\""<<endl;////////////////////////////////////////////////////////////////////////////////////////
                            }
                        //}

                    }
                }
            }
            line2=line3;
            i2=i3;
            i3++;
        }while(getline (infile,line3));


        istringstream iss(line3);
        iss >> subs;

        if(subs.compare("#pragma")==0)
        {
            iss >> subs;
            if(subs.compare("dhls")==0)
            {
                iss>>subs;
                if(subs.compare("operation")==0)
                {                    
                    cout<<argv[2]<<": Line "<<i3<<"  Last line can not have pragma for operation"<<endl;            
                }
            }
        }
        infile.close();

    }
    else
        cout << "Unable to open file"; 



    infile.open(argv[2]);
    if(infile.is_open())
    {
        getline (infile,line1);
        i1=1;
        check_new_linewp1;
        getline (infile,line2);
        i2=2;
        check_new_linewp2;
        getline (infile,line3);
        i3=3;


        do
        {
            check_new_linewp3;

     
            //////////Loop pragma checking in previous 2 lines
            istringstream iss(line3);
            iss >> subs;
            if(subs.compare("#pragma")==0)
            {
                iss >> subs;
                if(subs.compare("dhls")==0)
                {
                    iss>>subs;
                    if(subs.compare("loop")==0)
                    {
                        iss>>subs; //name of loop


                        string inf="";

                        while(iss>>tsubs)
                        {
                            inf=inf+" "+tsubs;
                        }

                        if(not(
                            (line1.find(subs)!=string::npos && (loop_finder(line1) || loop_finder(line2))) ||
                            (line2.find(subs)!=string::npos && loop_finder(line2))                
                        ))
                        {
                            cout<<argv[2]<<": Line "<<i3<<" Warning: Pragma \""<<inf<<"\" from \""<<trim(line3)<<"\" is not inside a loop named \""<<subs<<"\""<<endl;//////////////////////////////////////////////////////////////////////////////
                        }

                        //else
                        //{
                            //cout<<line3<<"  "<<pragma_present(inf, "loop.txt")<<endl;
                            if(!pragma_present(inf,"loop.txt"))
                            {
                                cout<<argv[2]<<": Line "<<i3<<" Warning: pragma \""<<inf<<"\" not implemented from \""<<trim(line3)<<"\""<<endl;////////////////////////////////////////////////////////////////////////////////////////
                            }
                        //}

                    }
                }
            }


            //////////Func pragma checking in previous 2 lines
            istringstream iss3(line3);
            iss3 >> subs;
            if(subs.compare("#pragma")==0)
            {
                iss3 >> subs;
                if(subs.compare("dhls")==0)
                {
                    iss3>>subs;
                    if(subs.compare("func")==0)
                    {
                        iss3>>subs; //name of func

                        string inf="";

                        while(iss3>>tsubs)
                        {
                            inf=inf+" "+tsubs;
                        }

                        if(
                            line1.find(subs)==string::npos && line2.find(subs)==string::npos        
                        )
                        {

                            cout<<argv[2]<<": Line "<<i3<<" Warning: Pragma \""<<inf<<"\" from \""<<trim(line3)<<"\" is not inside a function named \""<<subs<<"\""<<endl;//////////////////////////////////////////////////////////////////////////////
                            //cout<<"Not inside a function "<<line3<<endl;
                        }

                        //else
                        //{
                            
                            if(!pragma_present(inf,"func.txt"))
                            {
                                cout<<argv[2]<<": Line "<<i3<<" Warning: Pragma \""<<inf<<"\" not implemented  \""<<trim(line3)<<"\""<<endl;////////////////////////////////////////////////////////////////////////////////////////
                                //cout<<"Warning: pragma not implemented "<<line2<<endl;
                            }
                        //}                        

                    }
                }
            }

            line1=line2;
            line2=line3;
            i1=i2;
            i2=i3;
            i3++;
        }while(getline (infile,line3));

        infile.close();

    }
    else
        cout << "Unable to open file";    
}
