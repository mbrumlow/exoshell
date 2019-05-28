
#ifndef EXOTERM_TERM_TABLE_H
#define EXOTERM_TERM_TABLE_H

#define CASE_PRINT 0
#define CASE_ESC 1
#define CASE_7BIT 2
#define CASE_OSC 3
#define CASE_CSI 4
#define CSI_CUU_FILTER 5
#define CSI_CUD_FILTER 6
#define CSI_CUP_FILTER 7
#define CASE_ED_FILTER 8
#define CASE_EL_FITLER 9

#define CASE_LOG 254
#define CASE_TODO 255

static const char esc_start[] = {
  0, // 0
  0, // 1
  0, // 2
  0, // 3
  0, // 4
  0, // 5
  0, // 6
  0, // 7
  1, // 8  BS
  1, // 9  HT
  1, // 10 LF
  1, // 11 VT
  1, // 12 FF
  1, // 13 CR
  0, // 14
  0, // 15
  0, // 16
  0, // 17
  0, // 18
  0, // 19
  0, // 20
  0, // 21
  0, // 22
  0, // 23
  0, // 24
  0, // 25
  0, // 26
  1, // 27 ESC
  0, // 28
  0, // 29
  0, // 30
  0, // 31
  0, // 32
  0, // 33
  0, // 34
  0, // 35
  0, // 36
  0, // 37
  0, // 38
  0, // 39
  0, // 40
  0, // 41
  0, // 42
  0, // 43
  0, // 44
  0, // 45
  0, // 46
  0, // 47
  0, // 48
  0, // 49
  0, // 50
  0, // 51
  0, // 52
  0, // 53
  0, // 54
  0, // 55
  0, // 56
  0, // 57
  0, // 58
  0, // 59
  0, // 60
  0, // 61
  0, // 62
  0, // 63
  0, // 64
  0, // 65
  0, // 66
  0, // 67
  0, // 68
  0, // 69
  0, // 70
  0, // 71
  0, // 72
  0, // 73
  0, // 74
  0, // 75
  0, // 76
  0, // 77
  0, // 78
  0, // 79
  0, // 80
  0, // 81
  0, // 82
  0, // 83
  0, // 84
  0, // 85
  0, // 86
  0, // 87
  0, // 88
  0, // 89
  0, // 90
  0, // 91
  0, // 92
  0, // 93
  0, // 94
  0, // 95
  0, // 96
  0, // 97
  0, // 98
  0, // 99
  0, // 100
  0, // 101
  0, // 102
  0, // 103
  0, // 104
  0, // 105
  0, // 106
  0, // 107
  0, // 108
  0, // 109
  0, // 110
  0, // 111
  0, // 112
  0, // 113
  0, // 114
  0, // 115
  0, // 116
  0, // 117
  0, // 118
  0, // 119
  0, // 120
  0, // 121
  0, // 122
  0, // 123
  0, // 124
  0, // 125
  0, // 126
  0, // 127
  0, // 128
  0, // 129
  0, // 130
  0, // 131
  0, // 132
  0, // 133
  0, // 134
  0, // 135
  0, // 136
  0, // 137
  0, // 138
  0, // 139
  0, // 140
  0, // 141
  0, // 142
  0, // 143
  1, // 144 DSC
  0, // 145
  0, // 146
  0, // 147
  0, // 148
  0, // 149
  1, // 150 SPA
  1, // 151 EPA
  1, // 152 SOS
  0, // 153
  0, // 154
  1, // 155 CSI
  1, // 156 ST
  1, // 157 OCS
  0, // 156
  0, // 157
  1, // 158 PM
  1, // 159 APC
  0, // 160
  0, // 161
  0, // 162
  0, // 163
  0, // 164
  0, // 165
  0, // 166
  0, // 167
  0, // 168
  0, // 169
  0, // 170
  0, // 171
  0, // 172
  0, // 173
  0, // 174
  0, // 175
  0, // 176
  0, // 177
  0, // 178
  0, // 179
  0, // 180
  0, // 181
  0, // 182
  0, // 183
  0, // 184
  0, // 185
  0, // 186
  0, // 187
  0, // 188
  0, // 189
  0, // 190
  0, // 191
  0, // 192
  0, // 193
  0, // 194
  0, // 195
  0, // 196
  0, // 197
  0, // 198
  0, // 199
  0, // 200
  0, // 201
  0, // 202
  0, // 203
  0, // 204
  0, // 205
  0, // 206
  0, // 207
  0, // 208
  0, // 209
  0, // 210
  0, // 211
  0, // 212
  0, // 213
  0, // 214
  0, // 215
  0, // 216
  0, // 217
  0, // 218
  0, // 219
  0, // 220
  0, // 221
  0, // 222
  0, // 223
  0, // 224
  0, // 225
  0, // 226
  0, // 227
  0, // 228
  0, // 229
  0, // 230
  0, // 231
  0, // 232
  0, // 233
  0, // 234
  0, // 235
  0, // 236
  0, // 237
  0, // 238
  0, // 239
  0, // 240
  0, // 241
  0, // 242
  0, // 243
  0, // 244
  0, // 245
  0, // 246
  0, // 247
  0, // 248
  0, // 249
  0, // 250
  0, // 251
  0, // 252
  0, // 253
  0, // 254
  0, // 255
  0, // 256
};

static const char esc_table[] = {
  CASE_PRINT, // 0
  CASE_PRINT, // 1
  CASE_PRINT, // 2
  CASE_PRINT, // 3
  CASE_PRINT, // 4
  CASE_PRINT, // 5
  CASE_PRINT, // 6
  CASE_PRINT, // 7
  CASE_PRINT, // 8
  CASE_PRINT, // 9
  CASE_PRINT, // 10
  CASE_PRINT, // 11
  CASE_PRINT, // 12
  CASE_PRINT, // 13
  CASE_PRINT, // 14
  CASE_PRINT, // 15
  CASE_PRINT, // 16
  CASE_PRINT, // 17
  CASE_PRINT, // 18
  CASE_PRINT, // 19
  CASE_PRINT, // 20
  CASE_PRINT, // 21
  CASE_PRINT, // 22
  CASE_PRINT, // 23
  CASE_PRINT, // 24
  CASE_PRINT, // 25
  CASE_PRINT, // 26
  CASE_PRINT, // 27
  CASE_PRINT, // 28
  CASE_PRINT, // 29
  CASE_PRINT, // 30
  CASE_PRINT, // 31
  CASE_7BIT,  // 32 SP
  CASE_PRINT, // 33
  CASE_PRINT, // 34
  CASE_7BIT,  // 35 #
  CASE_PRINT, // 36
  CASE_7BIT,  // 37 %
  CASE_PRINT, // 38
  CASE_PRINT, // 39
  CASE_7BIT,  // 40 (
  CASE_7BIT,  // 41 )
  CASE_7BIT,  // 42 *
  CASE_7BIT,  // 43 +
  CASE_PRINT, // 44
  CASE_PRINT, // 45
  CASE_PRINT, // 46
  CASE_PRINT, // 47
  CASE_PRINT, // 48
  CASE_PRINT, // 49
  CASE_PRINT, // 50
  CASE_PRINT, // 51
  CASE_PRINT, // 52
  CASE_PRINT, // 53
  CASE_PRINT, // 54
  CASE_PRINT, // 55 7
  CASE_PRINT, // 56 8 
  CASE_PRINT, // 57  
  CASE_PRINT, // 58
  CASE_PRINT, // 59
  CASE_PRINT, // 60
  CASE_PRINT, // 61 = 
  CASE_PRINT, // 62 >
  CASE_PRINT, // 63
  CASE_PRINT, // 64
  CASE_PRINT, // 65
  CASE_PRINT, // 66
  CASE_PRINT, // 67
  CASE_TODO,  // 68 D
  CASE_TODO,  // 69 E
  CASE_PRINT, // 70 
  CASE_PRINT, // 71
  CASE_TODO,  // 72 H
  CASE_PRINT, // 73
  CASE_PRINT, // 74
  CASE_PRINT, // 75
  CASE_PRINT, // 76
  CASE_TODO,  // 77 M
  CASE_TODO,  // 78 N
  CASE_TODO,  // 79 O
  CASE_TODO,  // 80 P
  CASE_PRINT, // 81
  CASE_PRINT, // 82
  CASE_PRINT, // 83
  CASE_PRINT, // 84
  CASE_PRINT, // 85
  CASE_TODO,  // 86 V
  CASE_TODO,  // 87 W
  CASE_TODO,  // 88 X 
  CASE_TODO,  // 89 Y
  CASE_TODO,  // 90 Z
  CASE_CSI,   // 91 [
  CASE_TODO,  // 92 BACKSLASH
  CASE_OSC,  // 93 ]
  CASE_TODO,  // 94 ^
  CASE_TODO,  // 95 _
  CASE_PRINT, // 96
  CASE_PRINT, // 97
  CASE_PRINT, // 98
  CASE_PRINT, // 99 c
  CASE_PRINT, // 100
  CASE_PRINT, // 101
  CASE_PRINT, // 102
  CASE_PRINT, // 103
  CASE_PRINT, // 104
  CASE_PRINT, // 105
  CASE_PRINT, // 106
  CASE_PRINT, // 107
  CASE_PRINT, // 108
  CASE_PRINT, // 109
  CASE_PRINT, // 110
  CASE_PRINT, // 111
  CASE_PRINT, // 112
  CASE_PRINT, // 113
  CASE_PRINT, // 114
  CASE_PRINT, // 115
  CASE_PRINT, // 116
  CASE_PRINT, // 117
  CASE_PRINT, // 118
  CASE_PRINT, // 119
  CASE_PRINT, // 120
  CASE_PRINT, // 121
  CASE_PRINT, // 122
  CASE_PRINT, // 123
  CASE_PRINT, // 124
  CASE_PRINT, // 125
  CASE_PRINT, // 126
  CASE_PRINT, // 127
  CASE_PRINT, // 128
  CASE_PRINT, // 129
  CASE_PRINT, // 130
  CASE_PRINT, // 131
  CASE_PRINT, // 132
  CASE_PRINT, // 133
  CASE_PRINT, // 134
  CASE_PRINT, // 135
  CASE_PRINT, // 136
  CASE_PRINT, // 137
  CASE_PRINT, // 138
  CASE_PRINT, // 139
  CASE_PRINT, // 140
  CASE_PRINT, // 141
  CASE_PRINT, // 142
  CASE_PRINT, // 143
  CASE_PRINT, // 144
  CASE_PRINT, // 145
  CASE_PRINT, // 146
  CASE_PRINT, // 147
  CASE_PRINT, // 148
  CASE_PRINT, // 149
  CASE_PRINT, // 150
  CASE_PRINT, // 151
  CASE_PRINT, // 152
  CASE_PRINT, // 153
  CASE_PRINT, // 154
  CASE_PRINT, // 155
  CASE_PRINT, // 156
  CASE_PRINT, // 157
  CASE_PRINT, // 158
  CASE_PRINT, // 159
  CASE_PRINT, // 160
  CASE_PRINT, // 161
  CASE_PRINT, // 162
  CASE_PRINT, // 163
  CASE_PRINT, // 164
  CASE_PRINT, // 165
  CASE_PRINT, // 166
  CASE_PRINT, // 167
  CASE_PRINT, // 168
  CASE_PRINT, // 169
  CASE_PRINT, // 170
  CASE_PRINT, // 171
  CASE_PRINT, // 172
  CASE_PRINT, // 173
  CASE_PRINT, // 174
  CASE_PRINT, // 175
  CASE_PRINT, // 176
  CASE_PRINT, // 177
  CASE_PRINT, // 178
  CASE_PRINT, // 179
  CASE_PRINT, // 180
  CASE_PRINT, // 181
  CASE_PRINT, // 182
  CASE_PRINT, // 183
  CASE_PRINT, // 184
  CASE_PRINT, // 185
  CASE_PRINT, // 186
  CASE_PRINT, // 187
  CASE_PRINT, // 188
  CASE_PRINT, // 189
  CASE_PRINT, // 190
  CASE_PRINT, // 191
  CASE_PRINT, // 192
  CASE_PRINT, // 193
  CASE_PRINT, // 194
  CASE_PRINT, // 195
  CASE_PRINT, // 196
  CASE_PRINT, // 197
  CASE_PRINT, // 198
  CASE_PRINT, // 199
  CASE_PRINT, // 200
  CASE_PRINT, // 201
  CASE_PRINT, // 202
  CASE_PRINT, // 203
  CASE_PRINT, // 204
  CASE_PRINT, // 205
  CASE_PRINT, // 206
  CASE_PRINT, // 207
  CASE_PRINT, // 208
  CASE_PRINT, // 209
  CASE_PRINT, // 210
  CASE_PRINT, // 211
  CASE_PRINT, // 212
  CASE_PRINT, // 213
  CASE_PRINT, // 214
  CASE_PRINT, // 215
  CASE_PRINT, // 216
  CASE_PRINT, // 217
  CASE_PRINT, // 218
  CASE_PRINT, // 219
  CASE_PRINT, // 220
  CASE_PRINT, // 221
  CASE_PRINT, // 222
  CASE_PRINT, // 223
  CASE_PRINT, // 224
  CASE_PRINT, // 225
  CASE_PRINT, // 226
  CASE_PRINT, // 227
  CASE_PRINT, // 228
  CASE_PRINT, // 229
  CASE_PRINT, // 230
  CASE_PRINT, // 231
  CASE_PRINT, // 232
  CASE_PRINT, // 233
  CASE_PRINT, // 234
  CASE_PRINT, // 235
  CASE_PRINT, // 236
  CASE_PRINT, // 237
  CASE_PRINT, // 238
  CASE_PRINT, // 239
  CASE_PRINT, // 240
  CASE_PRINT, // 241
  CASE_PRINT, // 242
  CASE_PRINT, // 243
  CASE_PRINT, // 244
  CASE_PRINT, // 245
  CASE_PRINT, // 246
  CASE_PRINT, // 247
  CASE_PRINT, // 248
  CASE_PRINT, // 249
  CASE_PRINT, // 250
  CASE_PRINT, // 251
  CASE_PRINT, // 252
  CASE_PRINT, // 253
  CASE_PRINT, // 254
  CASE_PRINT, // 255
};

static const char csi_table[] = {
  CASE_CSI, // 0
  CASE_CSI, // 1
  CASE_CSI, // 2
  CASE_CSI, // 3
  CASE_CSI, // 4
  CASE_CSI, // 5
  CASE_CSI, // 6
  CASE_CSI, // 7
  CASE_CSI, // 8
  CASE_CSI, // 9
  CASE_CSI, // 10
  CASE_CSI, // 11
  CASE_CSI, // 12
  CASE_CSI, // 13
  CASE_CSI, // 14
  CASE_CSI, // 15
  CASE_CSI, // 16
  CASE_CSI, // 17
  CASE_CSI, // 18
  CASE_CSI, // 19
  CASE_CSI, // 20
  CASE_CSI, // 21
  CASE_CSI, // 22
  CASE_CSI, // 23
  CASE_CSI, // 24
  CASE_CSI, // 25
  CASE_CSI, // 26
  CASE_CSI, // 27
  CASE_CSI, // 28
  CASE_CSI, // 29
  CASE_CSI, // 30
  CASE_CSI, // 31
  CASE_CSI, // 32
  CASE_CSI, // 33
  CASE_CSI, // 34
  CASE_CSI, // 35
  CASE_CSI, // 36
  CASE_CSI, // 37
  CASE_CSI, // 38
  CASE_CSI, // 39
  CASE_CSI, // 40
  CASE_CSI, // 41
  CASE_CSI, // 42
  CASE_CSI, // 43
  CASE_CSI, // 44
  CASE_CSI, // 45
  CASE_CSI, // 46
  CASE_CSI, // 47
  CASE_CSI, // 48
  CASE_CSI, // 49
  CASE_CSI, // 50
  CASE_CSI, // 51
  CASE_CSI, // 52
  CASE_CSI, // 53
  CASE_CSI, // 54
  CASE_CSI, // 55
  CASE_CSI, // 56
  CASE_CSI, // 57
  CASE_CSI, // 58
  CASE_CSI, // 59
  CASE_CSI, // 60
  CASE_CSI, // 61
  CASE_CSI, // 62
  CASE_CSI, // 63
  CASE_PRINT, // 64 @ 
  CSI_CUU_FILTER, // 65 A
  CSI_CUD_FILTER, // 66 B
  CASE_PRINT, // 67 C
  CASE_PRINT, // 68 D
  CASE_PRINT, // 69 E
  CASE_PRINT, // 70 F
  CASE_PRINT, // 71 G
  CSI_CUP_FILTER, // 72 H 
  CASE_PRINT, // 73 I
  CASE_ED_FILTER, // 74 J 
  CASE_EL_FITLER, // 75 K 
  CASE_PRINT, // 76 L
  CASE_PRINT, // 77 M
  CASE_CSI, // 78 
  CASE_CSI, // 79
  CASE_PRINT, // 80 P
  CASE_CSI, // 81 
  CASE_CSI, // 82
  CASE_PRINT, // 83 S
  CASE_PRINT, // 84 T
  CASE_CSI, // 85
  CASE_CSI, // 86
  CASE_CSI, // 87
  CASE_PRINT, // 88 X
  CASE_CSI, // 89
  CASE_PRINT, // 90 Z
  CASE_CSI, // 91
  CASE_CSI, // 92
  CASE_CSI, // 93
  CASE_CSI, // 94
  CASE_CSI, // 95
  CASE_CSI, // 96 `
  CASE_CSI, // 97
  CASE_PRINT, // 98 b
  CASE_PRINT, // 99 c
  CASE_PRINT, // 100 d
  CASE_CSI, // 101 
  CASE_PRINT, // 102 f
  CASE_PRINT, // 103 g
  CASE_PRINT, // 104 h
  CASE_PRINT, // 105 i
  CASE_CSI, // 106
  CASE_CSI, // 107
  CASE_LOG, // 108 l
  CASE_PRINT, // 109 m
  CASE_PRINT, // 110 n
  CASE_CSI, // 111 
  CASE_PRINT, // 112 p
  CASE_PRINT, // 113 q
  CASE_PRINT, // 114 r
  CASE_CSI, // 115
  CASE_PRINT, // 116 t
  CASE_PRINT, // 117 u
  CASE_PRINT, // 118 v
  CASE_PRINT, // 119 w
  CASE_PRINT, // 120 x
  CASE_CSI, // 121 
  CASE_PRINT, // 122 z
  CASE_PRINT, // 123 { 
  CASE_PRINT, // 124 |
  CASE_CSI, // 125
  CASE_CSI, // 126
  CASE_CSI, // 127
  CASE_CSI, // 128
  CASE_CSI, // 129
  CASE_CSI, // 130
  CASE_CSI, // 131
  CASE_CSI, // 132
  CASE_CSI, // 133
  CASE_CSI, // 134
  CASE_CSI, // 135
  CASE_CSI, // 136
  CASE_CSI, // 137
  CASE_CSI, // 138
  CASE_CSI, // 139
  CASE_CSI, // 140
  CASE_CSI, // 141
  CASE_CSI, // 142
  CASE_CSI, // 143
  CASE_CSI, // 144
  CASE_CSI, // 145
  CASE_CSI, // 146
  CASE_CSI, // 147
  CASE_CSI, // 148
  CASE_CSI, // 149
  CASE_CSI, // 150
  CASE_CSI, // 151
  CASE_CSI, // 152
  CASE_CSI, // 153
  CASE_CSI, // 154
  CASE_CSI, // 155
  CASE_CSI, // 156
  CASE_CSI, // 157
  CASE_CSI, // 158
  CASE_CSI, // 159
  CASE_CSI, // 160
  CASE_CSI, // 161
  CASE_CSI, // 162
  CASE_CSI, // 163
  CASE_CSI, // 164
  CASE_CSI, // 165
  CASE_CSI, // 166
  CASE_CSI, // 167
  CASE_CSI, // 168
  CASE_CSI, // 169
  CASE_CSI, // 170
  CASE_CSI, // 171
  CASE_CSI, // 172
  CASE_CSI, // 173
  CASE_CSI, // 174
  CASE_CSI, // 175
  CASE_CSI, // 176
  CASE_CSI, // 177
  CASE_CSI, // 178
  CASE_CSI, // 179
  CASE_CSI, // 180
  CASE_CSI, // 181
  CASE_CSI, // 182
  CASE_CSI, // 183
  CASE_CSI, // 184
  CASE_CSI, // 185
  CASE_CSI, // 186
  CASE_CSI, // 187
  CASE_CSI, // 188
  CASE_CSI, // 189
  CASE_CSI, // 190
  CASE_CSI, // 191
  CASE_CSI, // 192
  CASE_CSI, // 193
  CASE_CSI, // 194
  CASE_CSI, // 195
  CASE_CSI, // 196
  CASE_CSI, // 197
  CASE_CSI, // 198
  CASE_CSI, // 199
  CASE_CSI, // 200
  CASE_CSI, // 201
  CASE_CSI, // 202
  CASE_CSI, // 203
  CASE_CSI, // 204
  CASE_CSI, // 205
  CASE_CSI, // 206
  CASE_CSI, // 207
  CASE_CSI, // 208
  CASE_CSI, // 209
  CASE_CSI, // 210
  CASE_CSI, // 211
  CASE_CSI, // 212
  CASE_CSI, // 213
  CASE_CSI, // 214
  CASE_CSI, // 215
  CASE_CSI, // 216
  CASE_CSI, // 217
  CASE_CSI, // 218
  CASE_CSI, // 219
  CASE_CSI, // 220
  CASE_CSI, // 221
  CASE_CSI, // 222
  CASE_CSI, // 223
  CASE_CSI, // 224
  CASE_CSI, // 225
  CASE_CSI, // 226
  CASE_CSI, // 227
  CASE_CSI, // 228
  CASE_CSI, // 229
  CASE_CSI, // 230
  CASE_CSI, // 231
  CASE_CSI, // 232
  CASE_CSI, // 233
  CASE_CSI, // 234
  CASE_CSI, // 235
  CASE_CSI, // 236
  CASE_CSI, // 237
  CASE_CSI, // 238
  CASE_CSI, // 239
  CASE_CSI, // 240
  CASE_CSI, // 241
  CASE_CSI, // 242
  CASE_CSI, // 243
  CASE_CSI, // 244
  CASE_CSI, // 245
  CASE_CSI, // 246
  CASE_CSI, // 247
  CASE_CSI, // 248
  CASE_CSI, // 249
  CASE_CSI, // 250
  CASE_CSI, // 251
  CASE_CSI, // 252
  CASE_CSI, // 253
  CASE_CSI, // 254
  CASE_CSI, // 255
}; 
  
#endif // EXOTERM_TERM_TABLE_H
