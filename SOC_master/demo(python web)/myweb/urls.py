from rest_framework.routers import DefaultRouter
from . import views

router = DefaultRouter()
router.register("BOLIST", views.BOListModelViewSet, basename="BOLIST")
router.register("BInfo", views.BInfoModelViewSet, basename="BInfo")

urlpatterns = [

] + router.urls