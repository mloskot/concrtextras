using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.Windows.Threading;
using System.Threading;
using System.IO;
using Microsoft.Win32;
using System.Diagnostics;

namespace ParallelSortDemo
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {             
        /// <summary>
        /// Struct for encapsulating interop with unmanaged dlls (NativeSortDemo.dll).
        /// </summary>
        struct Sorters
        {
            [DllImport(@".\NativeSortDemo.dll")]
            extern static public double native_sort(
                int sortType, [MarshalAs(UnmanagedType.LPArray)]byte[] a, int size, int concurrency, double comparatorSize);
        };

        /// <summary>
        /// this array is shared to the unmanaged dlls.  The unmanaged dlls apply the sort algorithm on the fillArray values. 
        /// Displaying this array as a bitmap, will in effect display all the values that are calculated. 
        /// Displaying this while it is being calculated will show the animation as to how it is being filled.
        /// </summary>
        byte[] fillArray;
        int dimX = 1000; //array size. always assume a square painting surface
        int dimY = 1000;
        PixelFormat pf = PixelFormats.Bgra32;        
        bool done = false; //whether the delgated operation is done or not
        int renderCount = 0;

        //recording timetaken
        double timeTaken;
        double pplParallelSortTime;
        double pplParallelBufferedSortTime;
        double pplParallelRadixSortTime;
        double stdSortTime;

        // delegates used for asynchronously updating/displaying the fillArray.
        // Note: threads can not be used as WPF does not allow other threads to share its data structure
        public delegate void AppDelegate();
        public delegate void UpdateDelegate();

        enum SortSelection { StdSort=0, PplParallelSort, PplParallelBufferedSort, PplParallelRadixSort };
        SortSelection sortType; //workaround for accessing the radiobutton state (because of multithreading and WPF issues). 

        enum DataTypeSelection { Random, PreSorted, NearlySorted, Sawtooth };
        DataTypeSelection dataType; //workaround for accessing the listbox state (because of multithreading and WPF issues).

        /// <summary>
        /// Defines the amount of work done in the Comparator, passed to the sort algorithm
        /// </summary>
        double comparatorSize;

        /// <summary>
        /// Defines the concurrency index for the parallel sort algorithms
        /// </summary>
        int concurrency;

        /// <summary>
        /// Constructor
        /// </summary>
        public MainWindow()
        {
            InitializeComponent();
            SetUIData();
            ResetFill();
            SetRectBackground();
            if (!File.Exists(@".\NativeSortDemo.dll"))
            {
                MessageBox.Show("WARNING!!!\nNativeSortDemo.dll not found in the current App directory. Please verify that it has been built for the appropriate architecture & flavor and binplaced correctly?");
            }
        }

        /// <summary>
        /// paint the rectangle with using the fillArray.
        /// </summary>
        private void SetRectBackground()
        {
            int rawStride = (dimX * pf.BitsPerPixel + 7) / 8;
            BitmapSource bitmap = BitmapSource.Create(dimX, dimY,
                        96, 96, pf, null,
                        fillArray, rawStride);
            ImageBrush ib = new ImageBrush(bitmap);
            rect.Fill = ib; 
        }

        /// <summary>
        /// As long as the delegated async operation is not complete, continue to paint the Rectangle with the current fillArray.
        /// This is what makes the animated fill of the rectangle possible.
        /// </summary>
        private void UpdateBackground()
        {
            SetRectBackground();
            UpdateClocks();
            if (!done)
            {
                pnlConfig.IsEnabled = false;
                if (renderCount++ % 100 == 0) System.GC.GetTotalMemory(true);

                //Note: await in Async.Net may be better suited here
                doSortButton.Dispatcher.BeginInvoke(DispatcherPriority.Render, new AppDelegate(UpdateBackground));                
            }
            else
            {
                pnlConfig.IsEnabled = true;
            }

        }

        /// <summary>
        /// updated text boxes using the time variables
        /// </summary>
        private void UpdateClocks()
        {
            txtTime.Text = timeTaken.ToString("N3");
            txtPplParallelSort.Text = pplParallelSortTime.ToString("N3");
            txtPplParallelBufferedSort.Text = pplParallelBufferedSortTime.ToString("N3");
            txtPplParallelRadixSort.Text = pplParallelRadixSortTime.ToString("N3");
            txtStdSort.Text = stdSortTime.ToString("N3");

            if (pplParallelSortTime == 0 || stdSortTime == 0)
            {
                txtParallelSortSpeedup.Text = "N/A";
                lblSortGainLoss.Content = "0%";
                lblSortGainLoss.Foreground = new SolidColorBrush(Colors.Black);
            }
            else
            {
                txtParallelSortSpeedup.Text = (pplParallelSortTime / stdSortTime).ToString("N3");
                if (pplParallelSortTime > stdSortTime)
                {
                    lblSortGainLoss.Content = (((pplParallelSortTime - stdSortTime) / stdSortTime) * 100).ToString("N3") + "%"; 
                    lblSortGainLoss.Foreground = new SolidColorBrush(Colors.Red);
                }
                else if (pplParallelSortTime < stdSortTime)
                {
                    lblSortGainLoss.Content = (((stdSortTime - pplParallelSortTime) / pplParallelSortTime) * 100).ToString("N3") + "%";
                    lblSortGainLoss.Foreground = new SolidColorBrush(Colors.Green);
                }
            }

            if (pplParallelBufferedSortTime == 0 || stdSortTime == 0)
            {
                txtParallelBufferedSortSpeedup.Text = "N/A";
                lblBuffSortGainLoss.Content = "0%";
                lblBuffSortGainLoss.Foreground = new SolidColorBrush(Colors.Black);

            }
            else
            {
                txtParallelBufferedSortSpeedup.Text = (pplParallelBufferedSortTime / stdSortTime).ToString("N3");
                if (pplParallelBufferedSortTime > stdSortTime)
                {
                    lblBuffSortGainLoss.Content = (((pplParallelBufferedSortTime - stdSortTime) / stdSortTime) * 100).ToString("N3") + "%";
                    lblBuffSortGainLoss.Foreground = new SolidColorBrush(Colors.Red);
                }
                else if (pplParallelBufferedSortTime < stdSortTime)
                {
                    lblBuffSortGainLoss.Content = (((stdSortTime - pplParallelBufferedSortTime) / pplParallelBufferedSortTime) * 100).ToString("N3") + "%";
                    lblBuffSortGainLoss.Foreground = new SolidColorBrush(Colors.Green);
                }
            }

            if (pplParallelRadixSortTime == 0 || stdSortTime == 0)
            {
                txtParallelRadixSortSpeedup.Text = "N/A";
                lblRdxSortGainLoss.Content = "0%";
                lblRdxSortGainLoss.Foreground = new SolidColorBrush(Colors.Black);

            }
            else
            {
                txtParallelRadixSortSpeedup.Text = (pplParallelRadixSortTime / stdSortTime).ToString("N3");
                if (pplParallelRadixSortTime > stdSortTime)
                {
                    lblRdxSortGainLoss.Content = (((pplParallelRadixSortTime - stdSortTime) / stdSortTime) * 100).ToString("N3") + "%";
                    lblRdxSortGainLoss.Foreground = new SolidColorBrush(Colors.Red);
                }
                else if (pplParallelRadixSortTime < stdSortTime)
                {
                    lblRdxSortGainLoss.Content = (((stdSortTime - pplParallelRadixSortTime) / pplParallelRadixSortTime) * 100).ToString("N3") + "%";
                    lblRdxSortGainLoss.Foreground = new SolidColorBrush(Colors.Green);
                }
            }
        }

        /// <summary>
        /// Resets the drawing area according to the Data Type selected
        /// This internally may call into native std sort
        /// Note: The pixels are gray-scaled for better/clearer effects
        /// </summary>
        private void ResetFill()
        {
            int width = dimX;
            int height = dimY;
            int rawStride = (width * pf.BitsPerPixel + 7) / 8;
            fillArray = new byte[rawStride * height];

            switch (dataType)
            {
                case DataTypeSelection.Random:
                    {
                        //Fill the drawing area with random pixels
                        Random rnd = new Random(100);
                        for (int i = 0, index = 0; i < dimX; ++i)
                        {
                            for (int j = 0; j < dimY; ++j)
                            {
                                byte r = (byte)rnd.Next(256);
                                fillArray[index++] = r;
                                fillArray[index++] = r;
                                fillArray[index++] = r;
                                fillArray[index++] = 255;
                            }
                        }                        
                    }
                    break;
                case DataTypeSelection.PreSorted:
                    {
                        //Fill the drawing area with presorted pixels
                        Random rnd = new Random(100);
                        for (int i = 0, index = 0; i < dimX; ++i)
                        {
                            for (int j = 0; j < dimY; ++j)
                            {
                                byte r = (byte)rnd.Next(256);
                                fillArray[index++] = r;
                                fillArray[index++] = r;
                                fillArray[index++] = r;
                                fillArray[index++] = 255;

                            }
                        }
                        Sorters.native_sort((int)SortSelection.StdSort, fillArray, dimX * dimY, 1, 0);
                    }
                    break;
                case DataTypeSelection.NearlySorted:
                    {
                        //Fill the drawing area with nearly sorted (99% sorted) pixels
                        Random rnd = new Random(100);
                        for (int i = 0, index = 0; i < dimX; ++i)
                        {
                            for (int j = 0; j < dimY; ++j)
                            {
                                byte r = (byte)rnd.Next(256);
                                fillArray[index++] = r;
                                fillArray[index++] = r;
                                fillArray[index++] = r;
                                fillArray[index++] = 255;

                            }
                        }
                        Sorters.native_sort((int)SortSelection.StdSort, fillArray, dimX * dimY, 1, 0);

                        for (int i = 0; i < (fillArray.Length * 0.01); ++i)
                        {                            
                            int rndElem1 = 4 + rnd.Next(fillArray.Length - 9);
                            rndElem1 -= rndElem1 % 4;
                            int rndElem2 = 4 + rnd.Next(fillArray.Length - 9);
                            rndElem2 -= rndElem2 % 4;

                            for (int j = 0; j < 4; ++j)
                            {
                                byte temp = fillArray[rndElem1+j];
                                fillArray[rndElem1 + j] = fillArray[rndElem2 + j];
                                fillArray[rndElem2 + j] = temp;
                            }
                        }
                    }
                    break;
                case DataTypeSelection.Sawtooth:
                    {
                        //Fill the drawing area with pixels representing a sawtooth
                        int toothSize = fillArray.Length / (concurrency * 4);
                        byte[] tempArray = new byte[toothSize * 4];

                        Random rnd = new Random(4);
                        for (int i = 0, index = 0; i < toothSize; ++i)
                        {
                            byte r = (byte)rnd.Next(256);
                            tempArray[index++] = r;
                            tempArray[index++] = r;
                            tempArray[index++] = r;
                            tempArray[index++] = 255;

                        }
                        Sorters.native_sort((int)SortSelection.StdSort, tempArray, (dimX * dimY) / concurrency, 1, 0);

                        for (int i = 0; i < concurrency; ++i)
                        {
                            Array.Copy(tempArray, 0, fillArray, i * tempArray.Length, tempArray.Length);
                        }
                    }
                    break;
            }
        }

        /// <summary>
        /// Sets fields called by the Constructor 
        /// </summary>
        private void SetUIData()
        {
            lstDataSetType.Items.Add("Random");
            lstDataSetType.Items.Add("Pre Sorted");
            lstDataSetType.Items.Add("Nearly Sorted");
            lstDataSetType.Items.Add("SawTooth");

            txtConcurrency.Text = System.Environment.ProcessorCount.ToString();
            concurrency = System.Convert.ToInt32(txtConcurrency.Text);            
        }

        /// <summary>
        /// Event handler when the Sort button is clicked.
        /// 1. Sets up the state by looking at the radiobutton group.
        /// 2. Also copies state of the data type, concurrency index and the comparator size
        /// 3. Creates a delegate to process the native sort algorithm asynchnronously
        /// </summary>
        private void OnDoSortButton(object sender, RoutedEventArgs e)
        {
            ResetFill();
            SetRectBackground();
            if (rbParallelSort.IsChecked.Value == true)
                sortType = SortSelection.PplParallelSort;
            else if (rbParallelBufferedSort.IsChecked.Value == true)
                sortType = SortSelection.PplParallelBufferedSort;
            else if (rbParallelRadixSort.IsChecked.Value == true)
                sortType = SortSelection.PplParallelRadixSort;
            else if (rbStdSort.IsChecked.Value == true)
                sortType = SortSelection.StdSort;
                        
            dataType = (DataTypeSelection)lstDataSetType.SelectedIndex;

            concurrency = System.Convert.ToInt32(txtConcurrency.Text);
            if (concurrency <= 0)
            {
                concurrency = 1;
                txtConcurrency.Text = "1";
            }

            comparatorSize = sldComparatorSize.Value;

            done = false;

            AppDelegate sd = new AppDelegate(ExecuteSort);
            sd.BeginInvoke(null, null);
            UpdateBackground();
        }

        /// <summary>
        /// Reset all the measurements and times to zero
        /// </summary>
        private void ResetClocks(object sender, RoutedEventArgs e)
        {
            timeTaken = 0;
            pplParallelSortTime = 0;
            pplParallelBufferedSortTime = 0;
            pplParallelRadixSortTime = 0;
            stdSortTime = 0;
            UpdateClocks();
            ResetFill();
            SetRectBackground();
        }

        /// <summary>
        /// Changes the drawing data based on the data type choosen
        /// </summary>
        private void lstDataSetType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            dataType = (DataTypeSelection)lstDataSetType.SelectedIndex;
            if (!(txtConcurrency.Text).Equals(""))
                concurrency = System.Convert.ToInt32(txtConcurrency.Text);            
            ResetFill();
            SetRectBackground();
            ResetClocks(sender, e);
        }

        /// <summary>
        /// Based on the selected radiobutton, call the appropriate unmanaged sort.
        /// </summary>
        private void ExecuteSort()
        {
            switch (sortType)
            {
                case SortSelection.StdSort:
                    {
                        stdSortTime = timeTaken = Sorters.native_sort((int)SortSelection.StdSort, fillArray, dimX * dimY, concurrency, comparatorSize);
                    }
                    break;
                case SortSelection.PplParallelSort:
                    {
                        pplParallelSortTime = timeTaken = Sorters.native_sort((int)SortSelection.PplParallelSort, fillArray, dimX * dimY, concurrency, comparatorSize);
                    }
                    break;
                case SortSelection.PplParallelBufferedSort:
                    {
                        pplParallelBufferedSortTime = timeTaken = Sorters.native_sort((int)SortSelection.PplParallelBufferedSort, fillArray, dimX * dimY, concurrency, comparatorSize);
                    }
                    break;
                case SortSelection.PplParallelRadixSort:
                    {
                        pplParallelRadixSortTime = timeTaken = Sorters.native_sort((int)SortSelection.PplParallelRadixSort, fillArray, dimX * dimY, concurrency, comparatorSize);
                    }
                    break;
            }
            Thread.Sleep(1000); //Without explicit long sleep, there seems to be an rendering update issue. 
            done = true;
        }        
    }
}
