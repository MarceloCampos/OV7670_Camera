using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;
using System.Diagnostics;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace serial_p
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        static int width = 320;
        static int height = 240;
        static int Start_StopSize = 10;
        static int Control_Bytes = 0;
        static string StartString = "*RDY*";
        static string StopString = "*STP*";


        TimeSpan Start_Time;
        TimeSpan Stop_Time;

        public static byte RxBytes;

        public bool StartFlag = false;

        byte[] buffer = new byte[width * height + Start_StopSize + Control_Bytes];
        int bytesJaLidos = 0;
        int bytesParaLer;

        private void Form1_Load(object sender, EventArgs e)
        {
            serialPort1.PortName = Properties.Settings.Default.CommPort;
            serialPort1.BaudRate = Properties.Settings.Default.BaudRate;
            txtBaudRate.Text = Properties.Settings.Default.BaudRate.ToString();
            txtComPort.Text = Properties.Settings.Default.CommPort.ToUpper();

            serialPort1.Open();
        }

        private void SerialREceive(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            try
            {
                while (true)
                {
                    bytesParaLer = serialPort1.BytesToRead;
                    bytesJaLidos += serialPort1.Read(buffer, bytesJaLidos, bytesParaLer);

                    if (bytesParaLer == 0)
                        break;
                }

                string RxString = Encoding.ASCII.GetString(buffer);
                int xp = RxString.Length;

                if (!StartFlag && RxString.StartsWith(StartString))
                {
                    StartFlag = true;
                    Debug.Print("Start !!");
                    Start_Time = DateTime.Now.TimeOfDay;
                    //                lblStatus.Text = ("Recebendo...");
                    //                this.Refresh();
                    //               lblStatus.Refresh();
                }
                else if (StartFlag && RxString.EndsWith(StopString))
                {
                    StartFlag = false;
                    bytesJaLidos = 0;
                    Debug.Print("Stop !!");
                    //              lblStatus.Text = ("Aguardando Imagem...");
                    Stop_Time = DateTime.Now.TimeOfDay;

                    TimeSpan time_rx = Stop_Time-Start_Time;
                    Debug.Print("Tempo de Leitura: " + time_rx.ToString());

                    //               lblTime.Text = "Tempo: " + time_rx.ToString();

                    //this.Refresh();
                    //               lblStatus.Refresh();
                    //               lblTime.Refresh();

                    Analiza_Rx();
                }
                else if (!StartFlag)
                {
                    bytesJaLidos = 0;
                }

            }
            catch (Exception ix)
            {
                Debug.Print("Erro leitura ");
                //       lblStatus.Text = ("Erro leitura ");
                StartFlag = false;
                bytesJaLidos = 0;
            }

        }

        private void Analiza_Rx()
        {
            byte[] rawBytes = new byte[width * height];

            for (int i = 0; i < rawBytes.Length; i++)
            {
                rawBytes[i] = buffer[i + StartString.Length + Control_Bytes];
            }

            var ms = new System.IO.MemoryStream(rawBytes);

            Bitmap bitmap = new Bitmap(width, height, PixelFormat.Format8bppIndexed);
            BitmapData bitmapData = bitmap.LockBits(new Rectangle(0, 0, width, height),
                                            ImageLockMode.WriteOnly, bitmap.PixelFormat);

            Marshal.Copy(rawBytes, 0, bitmapData.Scan0, rawBytes.Length);
            bitmap.UnlockBits(bitmapData);

            var pal = bitmap.Palette;
            for (int i = 0; i < 256; i++) pal.Entries[i] = Color.FromArgb(i, i, i);
            bitmap.Palette = pal;

            Array.Clear(buffer, 0, buffer.Length);

            pctBox.Image = bitmap;
           // lblResol.Text = "Resolução: " + width.ToString() + " x " + height.ToString();
            //lblResol.Refresh();
        }

        static byte[] PadLines(byte[] bytes, int rows, int columns)
        {
            int currentStride = columns; // 3
            int newStride = columns;  // 4
            byte[] newBytes = new byte[newStride * rows];
            for (int i = 0; i < rows; i++)
                Buffer.BlockCopy(bytes, currentStride * i, newBytes, newStride * i, currentStride);
            return newBytes;
        }

    }
}
