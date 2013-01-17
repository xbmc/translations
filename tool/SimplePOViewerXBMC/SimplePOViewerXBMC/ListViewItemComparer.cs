/*
 *      Copyright (C) 2012 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

using System;
using System.Collections;
using System.Windows.Forms;

namespace SimplePOViewerXBMC
{
    class ListViewItemComparer : IComparer
    {
        private readonly int orderby;
        private readonly SortOrder order;
        private readonly int compare_as;

        public ListViewItemComparer(int column, SortOrder order, Type expected)
        {
            orderby = column;
            this.order = order;

            if (expected == typeof(int))
            {
                compare_as = 1;
            }
            else
            {
                if (expected == typeof(DateTime))
                {
                    compare_as = 2;
                }
                else
                {
                    compare_as = 0;
                }
            }


        }
        public int Compare(object p, object q)
        {
            ListViewItem x = (ListViewItem)p;
            ListViewItem y = (ListViewItem)q;

            int result;
            switch (compare_as)
            {
                case 0:
                    {
                        result = String.Compare(x.SubItems[orderby].Text,
                                                y.SubItems[orderby].Text);

                        break;
                    }
                case 1:
                    {
                        int ix = 0;
                        int iy = 0;
                        try
                        {
                            ix = int.Parse(x.SubItems[orderby].Text);
                            iy = int.Parse(y.SubItems[orderby].Text);

                            result = ix - iy;
                        }
                        catch
                        {
                            result = String.Compare(x.SubItems[orderby].Text,
                                                y.SubItems[orderby].Text);
                        }

                        break;
                    }
                case 2:
                    {
                        try
                        {
                            DateTime d1 = DateTime.Parse(x.SubItems[orderby].Text);
                            DateTime d2 = DateTime.Parse(y.SubItems[orderby].Text);

                            result = DateTime.Compare(d1, d2);
                        }
                        catch
                        {
                            result = string.Compare(x.SubItems[orderby].Text,
                                                    y.SubItems[orderby].Text);
                        }
                        break;
                    }
                default:
                    goto case 0;
            }


            return (order == SortOrder.Descending) ? -result : result;
        }
    }

}
