// define some bitmapped letters!  put them in program memory so you aren't using up valuable running memory.
prog_uchar letters_48[] PROGMEM = /* 0 */ { 0,1, 0,2, 0,3, 1,0, 1,4, 2,1, 2,2, 2,3, 9,9 }; 
prog_uchar letters_49[] PROGMEM = /* 1 */ { 0,0, 0,1, 0,2, 0,3, 0,4, 9,9 }; 
prog_uchar letters_50[] PROGMEM = /* 2 */ { 0,0, 0,3, 0,4, 1,0, 1,2, 1,4, 2,1, 2,4, 9,9 }; 
prog_uchar letters_51[] PROGMEM = /* 3 */ { 0,0, 0,4, 1,0, 1,2, 1,4, 2,0, 2,1, 2,2, 2,3, 2,3, 9,9 }; 
prog_uchar letters_52[] PROGMEM = /* 4 */ { 0,0, 0,1, 0,2, 1,2, 2,1, 2,2, 2,3, 2,4, 9,9 }; 
prog_uchar letters_53[] PROGMEM = /* 5 */ { 0,0, 0,1, 0,2, 0,4, 1,0, 1,2, 1,4, 2,0, 2,2, 2,3, 9,9 }; 
prog_uchar letters_54[] PROGMEM = /* 6 */ { 0,1, 0,2, 0,3, 1,0, 1,2, 1,4, 2,0, 2,2, 2,3, 9,9 }; 
prog_uchar letters_55[] PROGMEM = /* 7 */ { 0,0, 0,3, 0,4, 1,0, 1,2, 2,0, 2,1, 9,9 }; 
prog_uchar letters_56[] PROGMEM = /* 8 */ { 0,0, 0,1, 0,3, 0,4, 1,0, 1,2, 1,4, 2,0, 2,1, 2,3, 2,4, 9,9 }; 
prog_uchar letters_57[] PROGMEM = /* 9 */ { 0,1, 0,2, 0,4, 1,0, 1,2, 1,4, 2,1, 2,2, 2,3, 9,9 }; 
prog_uchar letters_58[] PROGMEM = /* : */ { 0,1, 0,3, 9,9 }; 
prog_uchar letters_65[] PROGMEM = /* A */ { 0,1, 0,2, 0,3, 0,4, 1,0, 1,2, 2,0, 2,2, 3,1, 3,2, 3,3, 3,4, 9,9 }; 
prog_uchar letters_66[] PROGMEM = /* B */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,0, 1,2, 1,4, 2,0, 2,2, 2,4, 3,1, 3,3, 9,9 }; 
prog_uchar letters_67[] PROGMEM = /* C */ { 0,1, 0,2, 0,3, 1,0, 1,4, 2,0, 2,4, 3,1, 3,3, 9,9 }; 
prog_uchar letters_68[] PROGMEM = /* D */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,0, 1,4, 2,0, 2,4, 3,1, 3,2, 3,3, 9,9 }; 
prog_uchar letters_69[] PROGMEM = /* E */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,0, 1,2, 1,4, 2,0, 2,2, 2,4, 3,0, 3,4, 9,9 }; 
prog_uchar letters_70[] PROGMEM = /* F */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,0, 1,2, 2,0, 2,2, 3,0, 9,9 }; 
prog_uchar letters_71[] PROGMEM = /* G */ { 0,1, 0,2, 0,3, 1,0, 1,4, 2,0, 2,2, 2,4, 3,0, 3,2, 3,3, 9,9 }; 
prog_uchar letters_72[] PROGMEM = /* H */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,2, 2,2, 3,0, 3,1, 3,2, 3,3, 3,4, 9,9 }; 
prog_uchar letters_73[] PROGMEM = /* I */ { 0,0, 0,4, 1,0, 1,1, 1,2, 1,3, 1,4, 2,0, 2,4, 9,9 }; 
prog_uchar letters_74[] PROGMEM = /* J */ { 0,3, 1,4, 2,4, 3,0, 3,1, 3,2, 3,3, 9,9 }; 
prog_uchar letters_75[] PROGMEM = /* K */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,2, 2,1, 2,3, 3,0, 3,4, 9,9 }; 
prog_uchar letters_76[] PROGMEM = /* L */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,4, 2,4, 9,9 }; 
prog_uchar letters_77[] PROGMEM = /* M */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,1, 2,2, 3,1, 4,0, 4,1, 4,2, 4,3, 4,4, 9,9  }; 
prog_uchar letters_78[] PROGMEM = /* N */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,1, 2,2, 3,3, 4,0, 4,1, 4,2, 4,3, 4,4, 9,9 }; 
prog_uchar letters_79[] PROGMEM = /* O */ { 0,1, 0,2, 0,3, 1,0, 1,4, 2,0, 2,4, 3,1, 3,2, 3,3, 9,9 }; 
prog_uchar letters_80[] PROGMEM = /* P */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,0, 1,2, 2,0, 2,2, 3,1, 9,9 }; 
prog_uchar letters_81[] PROGMEM = /* Q */ { 0,1, 0,2, 0,3, 1,0, 1,4, 2,0, 2,3, 2,4, 3,1, 3,2, 3,3, 3,4, 9,9 }; 
prog_uchar letters_82[] PROGMEM = /* R */ { 0,0, 0,1, 0,2, 0,3, 0,4, 1,0, 1,2, 2,0, 2,2, 2,3, 3,1, 3,4, 9,9 }; 
prog_uchar letters_83[] PROGMEM = /* S */ { 0,1, 0,4, 1,0, 1,2, 1,4, 2,0, 2,2, 2,4, 3,0, 3,3, 9,9 }; 
prog_uchar letters_84[] PROGMEM = /* T */ { 0,0, 1,0, 1,1, 1,2, 1,3, 1,4, 2,0, 9,9 }; 
prog_uchar letters_85[] PROGMEM = /* U */ { 0,0, 0,1, 0,2, 0,3, 1,4, 2,4, 3,0, 3,1, 3,2, 3,3, 9,9 }; 
prog_uchar letters_86[] PROGMEM = /* V */ { 0,0, 0,1, 1,2, 1,3, 2,4, 3,2, 3,3, 4,0, 4,1, 9,9 }; 
prog_uchar letters_87[] PROGMEM = /* W */ { 0,0, 0,1, 0,2, 0,3, 1,4, 2,0, 2,1, 2,2, 2,3, 3,4, 4,0, 4,1, 4,2, 4,3, 9,9 }; 
prog_uchar letters_88[] PROGMEM = /* X */ { 0,0, 0,4, 1,1, 1,3, 2,2, 3,1, 3,3, 4,0, 4,4, 9,9 }; 
prog_uchar letters_89[] PROGMEM = /* Y */ { 0,0, 1,1, 2,2, 2,3, 2,4, 3,1, 4,0, 9,9 }; 
prog_uchar letters_90[] PROGMEM = /* Z */ { 0,0, 0,3, 0,4, 1,0, 1,2, 1,4, 2,0, 2,1, 2,4, 9,9 }; 

// Basically an ASCII lookup table
prog_uchar* font[] = { letters_48, letters_49, letters_50, letters_51, letters_52, letters_53, letters_54, letters_55, letters_56, letters_57, letters_58, 0, 0, 0, 0, 0, 0, letters_65, letters_66, letters_67, letters_68, letters_69, letters_70, letters_71, letters_72, letters_73, letters_74, letters_75, letters_76, letters_77, letters_78, letters_79, letters_80, letters_81, letters_82, letters_83, letters_84, letters_85, letters_86, letters_87, letters_88, letters_89, letters_90 };

// Define the beginning and end of your available glyphs
uint16_t fontMin=48;
uint16_t fontMax=90;

