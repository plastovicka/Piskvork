import java.util.Random;

public class RandomAlg extends GomokuInterface{
	
	final int MAX_BOARD = 100;
	int[][] board = new int[MAX_BOARD][MAX_BOARD];
	Random rand = new Random();
	
	@Override
	public String brain_about() {
		return "name=\"Random\", author=\"Zsolt Szabó\", version=\"1.0\", country=\"Hungary\"";
	}
	
	@Override
	public void brain_init() {
		if (width<5 || height<5) {
			width = height = 0;
			System.out.println("ERROR - Minimal board size is: 5");
			return;
		}
		if (width>MAX_BOARD || height>MAX_BOARD) {
			width = height = 0;
			System.out.println("ERROR - Maximal board size is: " + MAX_BOARD);
			return;
		}
		System.out.println("OK");
	}
	
	@Override
	public void brain_turn() {
		int x, y;
		do {
		    x = rand.nextInt(width);
		    y = rand.nextInt(height);
		} while(!isFree(x,y));
		do_mymove(x,y);
	}
	
	@Override
	public void brain_my(int x, int y) {
		if(isFree(x,y)) {
		    board[x][y] = 1;
		} else {
			System.out.println("ERROR - my move ["+x+","+y+"]");
		}
	}
	
	@Override
	public void brain_opponents(int x, int y) {
		if(isFree(x,y)) {
		    board[x][y] = 2;
		} else {
			System.out.println("ERROR - opponents's move ["+x+","+y+"]");
		}	
	}
	
	@Override
	public void brain_block(int x, int y) {
		if(isFree(x,y)) {
		    board[x][y] = 3;
		} else {
			System.out.println("ERROR - winning move ["+x+","+y+"]");
		}
	}
	
	@Override
	public void brain_eval(int x, int y) {
	}
	
	@Override
	public void brain_end() {
	}
	
	@Override
	public  void brain_restart() {
		if (width == 0) {
			width = height = 20;
		}
		for (int x=0; x<width; x++) {
			for (int y=0; y<height; y++) {
				board[x][y] = 0;
			}
		}
		System.out.println("OK");
	}
	
	@Override
	public boolean brain_takeback(int x, int y) {
		if (board[x][y] != 0) {
			board[x][y] = 0;
			return true;
		}
		return false;
	}
	
	private boolean isFree(int x, int y) {
		return x>=0 && y>=0 && x<width && y<height && board[x][y]==0;
	}
	
}