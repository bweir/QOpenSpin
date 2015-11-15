#include "openspin.h"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QObject>
#include <QDir>


class QSpin
{
    QByteArray filename;
    QList<QByteArray> includes;

public:
    QSpin()
    {

    }

    ~QSpin()
    {

    }

    void setFile(const QString & filename)
    {
        this->filename = filename.toLocal8Bit();
        qDebug() << filename;
        AddFilePath(file());
    }

    char * file()
    {
        return filename.data();
    }

    void addIncludePath(const QString & path)
    {
        includes.append(path.toLocal8Bit());
        AddPath(includes.last().data());
    }

};

int main(int argc, char* argv[])
{
    char* outfile = NULL;
    bool bVerbose = false;
    bool bQuiet = false;
    bool bDocMode = false;
    bool bBinary  = true;
    unsigned int  eeprom_size = 32768;
    int s_nFilesAccessed = 0;
    s_pCompilerData = NULL;
    bool bFileTreeOutputOnly = false;
    bool bFileListOutputOnly = false;
    bool bDumpSymbols = false;
    s_bUnusedMethodElimination = false;

    bool s_bFinalCompile = false;

    QSpin spin;
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("OpenSpin");
    QCoreApplication::setOrganizationName("Parallax");
    QCoreApplication::setOrganizationDomain("www.parallax.com");
    QCoreApplication::setApplicationVersion(VERSION);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    parser.setApplicationDescription(
            QObject::tr("An open-source compiler for the Spin language"));

    QCommandLineOption includeDirectory(    QStringList() << "I" << "L",            QObject::tr("Add a directory to the include path"),             QObject::tr("DIR"));
    QCommandLineOption outputFile(          QStringList() << "o" << "output",       QObject::tr("Output filename"),                                 QObject::tr("FILE"));
    QCommandLineOption EEPROMSize(          QStringList() << "M" << "eeprom-size",  QObject::tr("Set EEPROM maximum size (up to 16777216 bytes)"),  QObject::tr("SIZE"));

    parser.addOption(includeDirectory);
    parser.addOption(outputFile);
    parser.addOption(EEPROMSize);

    QCommandLineOption outputBinary(        QStringList() << "b" << "binary",       QObject::tr("Output in binary format"));
    QCommandLineOption outputEEPROM(        QStringList() << "e" << "eeprom",       QObject::tr("Output in EEPROM format"));
    QCommandLineOption dumpDOCMode(         QStringList() << "d" << "doc",          QObject::tr("Output documentation"));
    QCommandLineOption outputObjectTree(    QStringList() << "t" << "tree",         QObject::tr("Output object file tree"));
    QCommandLineOption outputFilenameList(  QStringList() << "f" << "files",        QObject::tr("Output list of filenames"));
    QCommandLineOption quietMode(           QStringList() << "q" << "quiet",        QObject::tr("Quiet mode (suppress any non-error text)"));
    QCommandLineOption verboseMode(         QStringList() << "v" << "verbose",      QObject::tr("Verbose output"));
    QCommandLineOption symbolInformation(   QStringList() << "s" << "symbol",       QObject::tr("Dump PUB & CON symbol information for top object"));
    QCommandLineOption unusedMethodRemoval( QStringList() << "u" << "unused",       QObject::tr("Enable unused method removal (EXPERIMENTAL!)"));

    parser.addOption(outputBinary);
    parser.addOption(outputEEPROM);
    parser.addOption(dumpDOCMode);
    parser.addOption(outputObjectTree);
    parser.addOption(outputFilenameList);
    parser.addOption(quietMode);
    parser.addOption(verboseMode);
    parser.addOption(symbolInformation);
    parser.addOption(unusedMethodRemoval);

    parser.addPositionalArgument("object",  QObject::tr("Spin file to compile"), "OBJECT");

    parser.process(app);

    if (parser.isSet(outputBinary))         bBinary = true;
    if (parser.isSet(outputEEPROM))         bBinary = false;

    if (parser.isSet(dumpDOCMode))          bDocMode = true;
    if (parser.isSet(outputObjectTree))     bFileTreeOutputOnly = true;
    if (parser.isSet(outputFilenameList))   bFileListOutputOnly = true;
    if (parser.isSet(quietMode))            bQuiet = true;
    if (parser.isSet(verboseMode))          bVerbose = true;
    if (parser.isSet(symbolInformation))    bDumpSymbols = true;
    if (parser.isSet(unusedMethodRemoval))  s_bUnusedMethodElimination = true;

    QByteArray ba = parser.value(outputFile).toLocal8Bit();
    if (!parser.value(outputFile).isEmpty())
        outfile = ba.data();

    if (!parser.value(EEPROMSize).isEmpty())
    {
        eeprom_size = parser.value(EEPROMSize).toInt();
        if (eeprom_size > 16777216)
            return 1;
    }

    if (!parser.values(includeDirectory).isEmpty())
    {
        foreach(QString dir, parser.values(includeDirectory))
        {
            spin.addIncludePath(dir);
        }
    }

    // FILE TO COMPILE

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty())
    {
        QTextStream(stderr) << "No object given to compile." << endl;
        parser.showHelp();
        return 1;
    }

    if (positionalArguments.size() > 1)
    {
        QTextStream(stderr) << "Only one object can be compiled at once." << endl;
        parser.showHelp();
        return 1;
    }

    spin.setFile(positionalArguments.first());

    if (bFileTreeOutputOnly || bFileListOutputOnly || bDumpSymbols)
    {
        bQuiet = true;
    }

    // replace .spin with .binary
    //

    QFileInfo fi(spin.file());

    if (fi.suffix() != "spin")
    {
        QTextStream(stderr) << "ERROR: spinfile must have .spin extension. You passed in:" << spin.file() << "\n";
        CleanupMemory();
        return 1;
    }

    // output file name

    QString outputfile = fi.canonicalPath() + "/" + fi.completeBaseName() + ".";

    qDebug() << outputfile;

    if (bBinary)
        outputfile += "binary";
    else
        outputfile += "eeprom";

    if (outfile)
        outputfile = outfile;
    
    qDebug() << outputfile;


    if (!bQuiet)
        QTextStream(stdout) << parser.applicationDescription() << "\n";

    if (bFileTreeOutputOnly || !bQuiet)
        QTextStream(stdout) << QFileInfo(spin.file()).fileName() << "\n";

    if ( s_bUnusedMethodElimination )
    {
        InitUnusedMethodData();
    }

restart_compile:
    s_pCompilerData = InitStruct();
    s_pCompilerData->bUnusedMethodElimination = s_bUnusedMethodElimination;
    s_pCompilerData->bFinalCompile = s_bFinalCompile;

    s_pCompilerData->list = new char[ListLimit];
    s_pCompilerData->list_limit = ListLimit;
    memset(s_pCompilerData->list, 0, ListLimit);

    if (bDocMode && !bQuiet)
    {
        s_pCompilerData->doc = new char[DocLimit];
        s_pCompilerData->doc_limit = DocLimit;
        memset(s_pCompilerData->doc, 0, DocLimit);
    }
    else
    {
        s_pCompilerData->doc = 0;
        s_pCompilerData->doc_limit = 0;
    }
    s_pCompilerData->bBinary = bBinary;
    s_pCompilerData->eeprom_size = eeprom_size;

    // allocate space for obj based on eeprom size command line option
    s_pCompilerData->obj_limit = eeprom_size > min_obj_limit ? eeprom_size : min_obj_limit;
    s_pCompilerData->obj = new unsigned char[s_pCompilerData->obj_limit];

    // copy filename into obj_title, and chop off the .spin
    strcpy(s_pCompilerData->obj_title, spin.file());
    char* pExtension = strstr(&s_pCompilerData->obj_title[0], ".spin");
    if (pExtension != 0)
    {
        *pExtension = 0;
    }

    int nCompileIndex = 0;
    if (!CompileRecursively(spin.file(), bQuiet, bFileTreeOutputOnly, nCompileIndex))
    {
        CleanupMemory();
        return 1;
    }

    if (!bFileTreeOutputOnly && !bFileListOutputOnly && !bDumpSymbols)
    {
        if (!s_bFinalCompile && s_bUnusedMethodElimination)
        {
            FindUnusedMethods(s_pCompilerData);
            s_bFinalCompile = true;
            CleanupMemory(false);
            goto restart_compile;
        }
        unsigned char* pBuffer = NULL;
        int bufferSize = 0;
        if (ComposeRAM(&pBuffer, bufferSize, bBinary, eeprom_size))
        {
            FILE* pFile = fopen(outputfile.toLocal8Bit().data(), "wb");
            if (pFile)
            {
                fwrite(pBuffer, bufferSize, 1, pFile);
                fclose(pFile);
            }
        }
        else
        {
            CleanupMemory();
            return 1;
        }

        if (!bQuiet)
        {
           printf("Program size is %d bytes\n", bufferSize);
        }

        delete [] pBuffer;
    }

    if (bDumpSymbols)
    {
        for (int i = 0; i < s_pCompilerData->info_count; i++)
        {
            char szTemp[256];
            szTemp[0] = '*';
            szTemp[1] = 0;
            int length = 0;
            int start = 0;
            if (s_pCompilerData->info_type[i] == info_pub || s_pCompilerData->info_type[i] == info_pri)
            {
                length = s_pCompilerData->info_data3[i] - s_pCompilerData->info_data2[i];
                start = s_pCompilerData->info_data2[i];
            }
            else if (s_pCompilerData->info_type[i] != info_dat && s_pCompilerData->info_type[i] != info_dat_symbol)
            {
                length = s_pCompilerData->info_finish[i] - s_pCompilerData->info_start[i];
                start = s_pCompilerData->info_start[i];
            }

            if (length > 0 && length < 256)
            {
                strncpy(szTemp, &s_pCompilerData->source[start], length);
                szTemp[length] = 0;
            }

            switch(s_pCompilerData->info_type[i])
            {
                case info_con:
                    printf("CON, %s, %d\n", szTemp, s_pCompilerData->info_data0[i]);
                    break;
                case info_con_float:
                    printf("CONF, %s, %f\n", szTemp, *((float*)&(s_pCompilerData->info_data0[i])));
                    break;
                case info_pub_param:
                    {
                        char szTemp2[256];
                        szTemp2[0] = '*';
                        szTemp2[1] = 0;
                        length = s_pCompilerData->info_data3[i] - s_pCompilerData->info_data2[i];
                        start = s_pCompilerData->info_data2[i];
                        if (length > 0 && length < 256)
                        {
                            strncpy(szTemp2, &s_pCompilerData->source[start], length);
                            szTemp2[length] = 0;
                        }
                        printf("PARAM, %s, %s, %d, %d\n", szTemp2, szTemp, s_pCompilerData->info_data0[i], s_pCompilerData->info_data1[i]);
                    }
                    break;
                case info_pub:
                    printf("PUB, %s, %d, %d\n", szTemp, s_pCompilerData->info_data4[i] & 0xFFFF, s_pCompilerData->info_data4[i] >> 16);
                    break;
            }
        }
    }

    if (bFileListOutputOnly)
    {
        for (int i = 0; i < s_nFilesAccessed; i++)
        {
            for (int j = i+1; j < s_nFilesAccessed; j++)
            {
                if (strcmp(s_filesAccessed[i], s_filesAccessed[j]) == 0)
                {
                    s_filesAccessed[j][0] = 0;
                }
            }
        }

        for (int i = 0; i < s_nFilesAccessed; i++)
        {
            if (s_filesAccessed[i][0] != 0)
            {
                printf("%s\n", s_filesAccessed[i]);
            }
        }
    }

    if (bVerbose && !bQuiet)
    {
        // do stuff with list and/or doc here
        int listOffset = 0;
        while (listOffset < s_pCompilerData->list_length)
        {
            char* pTemp = strstr(&(s_pCompilerData->list[listOffset]), "\r");
            if (pTemp)
            {
                *pTemp = 0;
            }
            printf("%s\n", &(s_pCompilerData->list[listOffset]));
            if (pTemp)
            {
                *pTemp = 0x0D;
                listOffset += (pTemp - &(s_pCompilerData->list[listOffset])) + 1;
            }
            else
            {
                listOffset += strlen(&(s_pCompilerData->list[listOffset]));
            }
        }
    }

    if (bDocMode && !bQuiet)
    {
        // do stuff with list and/or doc here
        int docOffset = 0;
        while (docOffset < s_pCompilerData->doc_length)
        {
            char* pTemp = strstr(&(s_pCompilerData->doc[docOffset]), "\r");
            if (pTemp)
            {
                *pTemp = 0;
            }
            printf("%s\n", &(s_pCompilerData->doc[docOffset]));
            if (pTemp)
            {
                *pTemp = 0x0D;
                docOffset += (pTemp - &(s_pCompilerData->doc[docOffset])) + 1;
            }
            else
            {
                docOffset += strlen(&(s_pCompilerData->doc[docOffset]));
            }
        }
    }

    CleanupMemory();

    return 0;
}
