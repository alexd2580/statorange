#include <iostream>
#include <string>

#include "BarWriter.hpp"

// #include <cmath>
// #include <cstring>
// #include <map>
// #include <string>
//
// #include "StateItem.hpp"
// #include "output.hpp"

// void fixed_length_progress(
//     ostream& out,
//     uint8_t min_text_length,
//     float progress,
//     Separator left,
//     Separator right,
//     pair<string, string> const& colors_left,
//     pair<string, string> const& color_right,
//     string const& text)
// {
//     uint8_t total_length = 2 * (min_text_length + 2) + 1;
//     bool has_left_sep = left != Separator::none;
//     bool has_right_sep = right != Separator::none;
//     total_length += has_left_sep ? 1 : 0;
//     total_length += has_right_sep ? 1 : 0;
//
//     uint8_t left_length = (uint8_t)floor((float)total_length * progress);
//     uint8_t right_length = total_length - left_length;
//
//     bool text_is_left = progress > 0.5f;
//
//     // Left Part.
//     if(round(progress) == 0.0f)
//     {
//         separator(out, left, colors_right);
//     }
//     else
//     {
//         // If the progress is zero then the first separator is empty-colored.
//         separator(out, left, colors_left);
//         uint8_t left_to_fill = left_length - (no_left_sep ? 0 : 1);
//         if(text_is_left)
//         {
//             out << " " << text << " ";
//             left_to_fill -= text.length() + 2;
//         }
//         out << string(left_to_fill, ' ');
//     }
//
//     // Right part.
//     if(round(progress) == 1.0f)
//     {
//     }
//     else
//     {
//         uint8_t right_to_fill = right_length - (no_right_sep ? 0 : 1);
//     }
//
//     // Closing separator.
//     separator(out, right, Coloring::white_on_black);
// }

/**
 * Prints the buttons for switching workspaces.
 * Align them to the left side.
 */
// void echo_workspace_buttons(I3State& i3,
//                             WorkspaceGroup show_names,
//                             Output const& disp,
//                             bool focused)
// {
//     for(auto const& workspace_pair : disp.workspaces)
//     {
//         auto const workspace_ptr = workspace_pair.second;
//         auto const& workspace = *workspace_ptr;
//         bool visible = workspace_ptr.get() ==
//         disp.focused_workspace.get();
//
//         Coloring color = Coloring::inactive;
//         if(workspace.urgent)
//             color = Coloring::warn;
//         else if(focused && visible)
//             color = Coloring::active;
//         else if(visible)
//             color = Coloring::semiactive;
//
//         startButton("i3-msg workspace " + workspace.name);
//         separate(Separator::right, color);
//         cout << ' ' << workspace.name << ' ';
//
//         bool show_name =
//             show_names == WorkspaceGroup::all ||
//             (show_names == WorkspaceGroup::visible && visible) ||
//             (show_names == WorkspaceGroup::active && focused && visible);
//
//         if(show_name && workspace.focused_window_id != -1)
//         {
//             auto iter = i3.windows.find(workspace.focused_window_id);
//             if(iter != i3.windows.end())
//             {
//                 separate(Separator::right, color);
//                 cout << ' ' << iter->second.name << ' ';
//             }
//         }
//         separate(Separator::right, Coloring::white_on_black);
//         stopButton();
//     }
// }

// void echo_lemon(void)
// {
//     for(auto const& output_pair : i3.outputs)
//     {
//         auto& output = output_pair.second;
//         cout << "%{S" << (int)output->position << "}";
//         cout << "%{l}";
//         bool focused = i3.focused_output.get() == output.get();
//         echo_workspace_buttons(i3, show_names, *output, focused);
//         if(i3.mode.compare("default") != 0)
//         {
//             cout << ' ';
//             separate(Separator::left, Coloring::info);
//             cout << ' ' << i3.mode << ' ';
//             separate(Separator::right, Coloring::white_on_black);
//         }
//         cout << "%{l}";
//         cout << "%{r}";
//
//         StateItem::print_state();
//
//         cout << "%{r}";
//         cout << "%{S" << (int)output->position << "}";
//     }
//
//     cout << endl;
// }
} // namespace BarWriter
// WorkspaceGroup parse_workspace_group(string const& s)
// {
//     PARSE_CASE(WorkspaceGroup, all)
//     PARSE_CASE(WorkspaceGroup, visible)
//     PARSE_CASE(WorkspaceGroup, active)
//     return WorkspaceGroup::none;
// }
