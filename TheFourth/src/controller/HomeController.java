package controller;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.servlet.ModelAndView;

//@RequestMapping("/TheFourth")
@Controller
public class HomeController {
	/*
	 * 首页 返回至/views/page.jsp页面
	 * @return
	 */
	@RequestMapping("index")
	public ModelAndView index(){
		//create the model and view
		return new ModelAndView("home");
	}
}
