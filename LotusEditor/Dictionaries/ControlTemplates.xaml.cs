using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace LotusEditor.Dictionaries
{
    public partial class ControlTemplates : ResourceDictionary
    {
        private void OnTextBox_KeyDown(object sender, KeyEventArgs e)
        {
            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null) return;

            if (e.Key == Key.Enter)
            {
                if (textBox.Tag is ICommand command && command.CanExecute(textBox.Text))
                {
                    command.Execute(textBox.Text);
                }
                else
                {
                    exp.UpdateSource();
                }

                Keyboard.ClearFocus();
                e.Handled = true;
            }else if (e.Key == Key.Escape)
            {
                exp.UpdateTarget();
                Keyboard.ClearFocus();
            }
        }

        private void Close_OnButtonClick(object sender, RoutedEventArgs e)
        {
            var win = (Window)((FrameworkElement)sender).TemplatedParent;
            win.Close();
        }

        private void MaximizeRestore_OnButtonClick(object sender, RoutedEventArgs e)
        {
            var win = (Window)((FrameworkElement)sender).TemplatedParent;
            win.WindowState = win.WindowState == WindowState.Normal ? WindowState.Maximized : WindowState.Normal;
        }

        private void Minimize_OnButtonClick(object sender, RoutedEventArgs e)
        {
            var win = (Window)((FrameworkElement)sender).TemplatedParent;
            win.WindowState = WindowState.Minimized;
        }

        private void TextBoxRename_OnKeyDown(object sender, KeyEventArgs e)
        {
            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null) return;

            if (e.Key == Key.Enter)
            {
                if (textBox.Tag is ICommand command && command.CanExecute(textBox.Text))
                {
                    command.Execute(textBox.Text);
                }
                else
                {
                    exp.UpdateSource();
                }

                textBox.Visibility = Visibility.Collapsed;
                e.Handled = true;
            }
            else if (e.Key == Key.Escape)
            {
                exp.UpdateTarget();
                textBox.Visibility = Visibility.Collapsed;
            }
        }

        private void TextBoxRename_OnFocusLost(object sender, RoutedEventArgs e)
        {
            var textBox = sender as TextBox;
            if (!textBox.IsVisible) return;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null) return;
            exp.UpdateTarget();
            // textBox.MoveFocus(new TraversalRequest(FocusNavigationDirection.Previous));
            textBox.Visibility = Visibility.Collapsed;
        }
    }
}
