#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qfile.h>
#include <qdatetime.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void
MainWindow::status_update()
{
	FILE *fp;
        char buffer[256];
        char filecontent[4096];

        char version[128];


    //    QFont ft,ft1;

 //       ft.setPointSize(10);
  //      ft.setBold(1);
  //      ui->label->setFont(ft);
       // ui->label->setWordWrap(true);//×Ô¶¯»»ÐÐ
      //  ui->label->setAlignment(Qt::AlignTop);

       // ft.setPointSize(10);
        //ft.setBold(1);
       // ui->label2->setFont(ft);





        ui->label_2->setWordWrap(true);
        ui->label_2->setAlignment(Qt::AlignTop);


        ui->label_3->setWordWrap(true);
        ui->label_3->setAlignment(Qt::AlignTop);

        setStyleSheet("background-color:rgb(94,56,192)");
        //setStyleSheet(QString::fromUtf8( "font: 10pt \"wenquanyi\";background-color:rgb(94,56,192)\n"));


        QDateTime time;
				QString str;
	while (1)
	{

            time = QDateTime::currentDateTime();
            str = time.toString("yyyy-MM-dd hh:mm:ss");
            ui->label_time->setText(str);


            //set boxid
            memset(buffer,0,sizeof(buffer));
            fp = fopen ("../disp/box_id", "r");
            if (fp !=  NULL)
            {
                fgets (buffer, sizeof (buffer) - 1, fp);
                fclose(fp);
                ui->label->setText(buffer);
            }

            sleep (1);





            //set box_status
            memset(buffer,0,sizeof(buffer));
            memset(filecontent,0,sizeof(filecontent));
            fp = fopen ("../disp/box_status", "r");
            if (fp !=  NULL)
            {
                while (fgets (buffer, sizeof (buffer) - 1, fp) != NULL)
                {
                    strcat(filecontent,buffer);
                }
                fclose(fp);
                ui->label_2->setText(filecontent);
            }


            //set error
            memset(buffer,0,sizeof(buffer));
            memset(filecontent,0,sizeof(filecontent));
            fp = fopen ("../disp/error", "r");
            if (fp !=  NULL)
            {
                while (fgets (buffer, sizeof (buffer) - 1, fp) != NULL)
                {
                    strcat(filecontent,buffer);
                }
                fclose(fp);
                ui->label_3->setText(filecontent);
            }


            //set vai
            memset(version,0,sizeof(version));

            memset(buffer,0,sizeof(buffer));
            fp = fopen ("../disp/vai", "a+");
            fgets (buffer, sizeof (buffer) - 1, fp);
            fclose(fp);
            ui->label_vai->setText(buffer);

            memset(buffer,0,sizeof(buffer));
            fp = fopen ("../disp/vud", "a+");
            fgets (buffer, sizeof (buffer) - 1, fp);
            fclose(fp);
            ui->label_vud->setText(buffer);

            memset(buffer,0,sizeof(buffer));
            fp = fopen ("../disp/vul", "a+");
            fgets (buffer, sizeof (buffer) - 1, fp);
            fclose(fp);
            ui->label_vul->setText(buffer);


            //set upload
            memset(buffer,0,sizeof(buffer));
            fp = fopen ("../disp/upload", "a+");
            fgets (buffer, sizeof (buffer) - 1, fp);
            fclose(fp);
            ui->label_upload->setText(buffer);

         }

}

