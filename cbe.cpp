// Written on ubuntu environment
#include <iostream>
#include <curl/curl.h>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

using namespace std;

const string baseurl = "http://apps.cbe.com.et:100/";
static string web_response;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

int downloader(string tx, string fileN)
{
    CURL *curl_handle;

    FILE *pagefile;
    string tempURL = "http://apps.cbe.com.et:100/?id=" + tx;
    const char *temp = tempURL.c_str();
    char *url = strdup(temp);
    static const char *pagefilename = fileN.c_str();

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    pagefile = fopen(pagefilename, "wb");
    if (pagefile)
    {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
        curl_easy_perform(curl_handle);
        fclose(pagefile);
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    return 0;
}

void executeHTTPRequest(CURL *curl)
{
    CURLcode res;
    web_response = "";
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    web_response += (const char *)contents;
    web_response += '\n';
    return size * nmemb;
}

void sendPublicRequest(CURL *curl, string txRecipt)
{
    string url = baseurl;
    url += "?id=" + txRecipt;
    // cout << "url is: " << url << endl;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    executeHTTPRequest(curl);
}

void checkPayment(string tx)
{
    CURL *curl;
    string queryString;

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    struct curl_slist *chunk = NULL;
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    sendPublicRequest(curl, tx);

    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

void pdfDecoder(string fileN, string txType)
{

    string pdfPath = fileN;
    string outputFilePath = "output.txt";

    string command = "bin64/pdftotext " + pdfPath + " " + outputFilePath;
    int status = system(command.c_str());

    string finalResult = "";
    ifstream outputFile(outputFilePath);
    if (outputFile.is_open())
    {
        string textContent;
        string line;

        int counter = 0;

        while (getline(outputFile, line))
        {
            textContent = line;

            if (counter == 27)
            {
                string temp = textContent;
                string final;
                for (int i = 0; i < temp.length(); i++)
                {
                    if (temp[i] == ' ' && temp[i + 1] == '1')
                    {
                        break;
                    }
                    final += temp[i];
                }
                finalResult = "Payer Full Name: " + final + "\n";
            }
            else if (counter == 28 && txType == "c")
            {
                cout<<"--------transaction type is:  CREDIT  ----------"<<endl;
                finalResult += "Reciver: " + textContent + "\n";
            }
            else if (counter == 28 && txType == "d")
            {
                cout<<"--------transaction type is:  DEBIT  ----------"<<endl;
                string temp = textContent;
                string final;
                for (int i = 0; i < temp.length(); i++)
                {
                    if (temp[i] == ' ' && temp[i + 1] == 'F' && i > 5)
                    {
                        break;
                    }
                    final += temp[i];
                }
                finalResult += "Reciver: " + final + "\n";
            }
            else if (counter == 29 && txType == "c")
            {
                string temp = textContent;
                string final;
                for (int i = 0; i < temp.length(); i++)
                {
                    if (temp[i] == ' ' && temp[i + 1] == 'F' && i > 5)
                    {
                        break;
                    }
                    final += temp[i];
                }
                finalResult += "Payment Date: " + final + "\n";
            }
            else if (counter == 29 && txType == "d")
            {
                string temp = textContent;
                string final,date;
                bool jumper = false;
                for (int i = 0; i < temp.length(); i++)
                {
                    if (temp[i] == 'E' && temp[i + 1] == 'T' && temp[i + 2] == 'B')
                    {
                        i = i + 3;
                        jumper = true;
                    }
                    if (jumper)
                        final += temp[i];
                    else
                        date += temp[i];
                }
                finalResult += "Payment Date: " +date +"\n" + "Amount: "+ final;
            }

            // Topup 0914605312 ETB10.00
            else if (counter == 30 && txType == "c")
            {
                string temp = textContent;
                string final;
                bool jumper = false;
                for (int i = 0; i < temp.length(); i++)
                {
                    if (temp[i] == 'E' && temp[i + 1] == 'T' && temp[i + 2] == 'B')
                    {
                        i = i + 3;
                        jumper = true;
                    }

                    if (jumper)
                        final += temp[i];
                }
                finalResult += "Amount: " + final + " ETB\n";
            }
            counter++;
        }
        outputFile.close();
        cout << finalResult << endl;
        cout<<"-----------------------------------------------\n";
    }
    else
    {
        cout << "Failed to open output file." << endl;
    }
}


int main()
{
    // some transaction examples
    // debit FT23338ZBJBF15577218
    // credi FT23338C2NGC15577218
    string txType = "c";
    string txRecipt="FT23338C2NGC15577218";
    string fileName="cbe9.pdf";

    cout<<"enter transaction type.c or d? (credit or debit): ";
    cin>>txType;
    cout<<"enter the reference n.o: ";
    cin>>txRecipt;
    checkPayment(txRecipt);
 
    if (web_response[0] == 'Y')
    {
        cout << "===>not payed" << endl;
    }
    else
    {
        cout << "===>payed successfuly" << endl;
        cout << "Transaction recipt: "<<txRecipt<<endl<<endl;
        downloader(txRecipt, fileName);
        pdfDecoder(fileName, txType);
    } 
}
